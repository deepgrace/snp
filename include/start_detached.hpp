//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef START_DETACHED_HPP
#define START_DETACHED_HPP

#include <unifex/bind_back.hpp>
#include <unifex/tag_invoke.hpp>
#include <unifex/scope_guard.hpp>
#include <unifex/get_allocator.hpp>
#include <unifex/sender_concepts.hpp>

namespace snp {
namespace _start_detached {

using namespace unifex;

template <typename Alloc>
struct _start_detached_receiver final
{
    static_assert(std::is_same_v<std::byte, typename Alloc::value_type>);

    void set_value() noexcept
    {
        deleter_(std::move(alloc_), op_);
    }

    template <typename Error>
    [[noreturn]] void set_error(Error&& error) noexcept
    {
        std::terminate();
    }

    void set_done() noexcept
    {
        set_value();
    }

    friend Alloc tag_invoke(tag_t<get_allocator>, const _start_detached_receiver& receiver) noexcept(std::is_nothrow_copy_constructible_v<Alloc>)
    {
        return receiver.alloc_;
    }

    void* op_;
    void (*deleter_)(Alloc, void*) noexcept;

    UNIFEX_NO_UNIQUE_ADDRESS Alloc alloc_;
};

template <typename Alloc, typename T>
using rebind = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

template <typename Alloc>
using start_detached_receiver_t = _start_detached_receiver<rebind<Alloc, std::byte>>;

template <typename Sender, typename Alloc>
struct _start_detached_op final
{
    static_assert(std::is_same_v<std::byte, typename Alloc::value_type>);

    explicit _start_detached_op(Sender&& sender, const Alloc& alloc) noexcept(is_nothrow_connectable_v<Sender, start_detached_receiver_t<Alloc>>)
    : op_{unifex::connect(std::move(sender), start_detached_receiver_t<Alloc>{this, destroy, alloc})}
    {
    }

    _start_detached_op(_start_detached_op&&) = delete;

    ~_start_detached_op() = default;

    friend void tag_invoke(tag_t<start>, _start_detached_op& op) noexcept
    {
        unifex::start(op.op_);
    }

private:
    using op_t = connect_result_t<Sender, start_detached_receiver_t<Alloc>>;

    op_t op_;

    static void destroy(Alloc alloc, void* p) noexcept
    {
        using allocator_t = rebind<Alloc, _start_detached_op>;
        using traits_t = typename std::allocator_traits<allocator_t>;

        auto typed = static_cast<_start_detached_op*>(p);
        allocator_t allocator{std::move(alloc)};

        traits_t::destroy(allocator, typed);
        traits_t::deallocate(allocator, typed, 1);
    }
};

template <typename Sender, typename Alloc>
using start_detached_op_t = _start_detached_op<Sender, rebind<Alloc, std::byte>>;

struct _start_detached_fn
{
    template<typename Sender, typename Alloc = std::allocator<std::byte>>
    requires (sender<Sender> && is_allocator_v<Alloc> && sender_to<Sender, start_detached_receiver_t<Alloc>>)
    void operator()(Sender&& sender, const Alloc& alloc = {}) const
    {
        using op_t = start_detached_op_t<Sender, Alloc>;

        using allocator_t = rebind<Alloc, op_t>;
        using traits = std::allocator_traits<allocator_t>;

        allocator_t allocator{alloc};
        auto op = traits::allocate(allocator, 1);

        scope_guard g = [&] noexcept
        {
            traits::deallocate(allocator, op, 1);
        };

        traits::construct(allocator, op, std::forward<Sender>(sender),  allocator);

        g.release();
        unifex::start(*op);
    }

    template <typename Alloc = std::allocator<std::byte>>
    constexpr auto operator()(const Alloc& alloc = {}) const noexcept(is_nothrow_callable_v<tag_t<bind_back>, _start_detached_fn, const Alloc&>)
    -> std::enable_if_t<is_allocator_v<Alloc>, bind_back_result_t<_start_detached_fn, const Alloc&>>
    {
        return bind_back(*this, alloc);
    }
};

}

inline constexpr _start_detached::_start_detached_fn start_detached{};

}

#endif
