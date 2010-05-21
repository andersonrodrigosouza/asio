//
// detail/impl/posix_tss_ptr.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_POSIX_TSS_PTR_IPP
#define ASIO_DETAIL_IMPL_POSIX_TSS_PTR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/posix_tss_ptr.hpp"

#if defined(BOOST_HAS_PTHREADS) && !defined(ASIO_DISABLE_THREADS)

#include <boost/throw_exception.hpp>
#include "asio/error.hpp"
#include "asio/system_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

void posix_tss_ptr_create(pthread_key_t& key)
{
  int error = ::pthread_key_create(&key, 0);
  if (error != 0)
  {
    asio::system_error e(
        asio::error_code(error,
          asio::error::get_system_category()),
        "tss");
    boost::throw_exception(e);
  }
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(BOOST_HAS_PTHREADS) && !defined(ASIO_DISABLE_THREADS)

#endif // ASIO_DETAIL_IMPL_POSIX_TSS_PTR_IPP
