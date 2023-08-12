//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef SNP_VERSION_HPP
#define SNP_VERSION_HPP

#define SNP_STRINGIZE(T) #T

/*
 *   SNP_VERSION_NUMBER
 *
 *   Identifies the API version of snp.
 *   This is a simple integer that is incremented by one every
 *   time a set of code changes is merged to the master branch.
 */

#define SNP_VERSION_NUMBER 3
#define SNP_VERSION_STRING "snp/" SNP_STRINGIZE(SNP_VERSION_NUMBER)

#endif
