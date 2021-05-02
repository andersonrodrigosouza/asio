//
// cancellation_signal.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_CANCELLATION_SIGNAL_HPP
#define ASIO_CANCELLATION_SIGNAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cassert>
#include <new>
#include <utility>
#include "asio/detail/cstddef.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class cancellation_handler_base
{
public:
  virtual void call() = 0;
  virtual std::pair<void*, std::size_t> destroy() ASIO_NOEXCEPT = 0;

protected:
  ~cancellation_handler_base() {}
};

template <typename Context, typename Handler>
class cancellation_handler
  : public cancellation_handler_base
{
public:
  template <typename H, typename... Args>
  static cancellation_handler* create(std::pair<void*, std::size_t> mem,
      ASIO_MOVE_ARG(H) handler, ASIO_MOVE_ARG(Args)... args)
  {
    if (sizeof(cancellation_handler) > mem.second)
    {
      ::operator delete(mem.first);
      mem.first = ::operator new(sizeof(cancellation_handler));
      mem.second = sizeof(cancellation_handler);
    }

    try
    {
      return new (mem.first) cancellation_handler(
          mem.second, ASIO_MOVE_CAST(H)(handler),
          ASIO_MOVE_CAST(Args)(args)...);
    }
    catch (...)
    {
      ::operator delete(mem.first);
      throw;
    }
  }

  void call()
  {
    handler_(context_);
  }

  std::pair<void*, std::size_t> destroy() ASIO_NOEXCEPT
  {
    std::pair<void*, std::size_t> mem(this, size_);
    this->cancellation_handler::~cancellation_handler();
    return mem;
  }

  Context& context() ASIO_NOEXCEPT
  {
    return context_;
  }

private:
  template <typename H, typename... Args>
  cancellation_handler(std::size_t size,
      ASIO_MOVE_ARG(H) handler, ASIO_MOVE_ARG(Args)... args)
    : handler_(ASIO_MOVE_CAST(H)(handler)),
      context_(ASIO_MOVE_CAST(Args)(args)...),
      size_(size)
  {
  }

  ~cancellation_handler()
  {
  }

  Handler handler_;
  Context context_;
  std::size_t size_;
};

template <typename Handler>
class cancellation_handler<void, Handler>
  : public cancellation_handler_base
{
public:
  template <typename H>
  static cancellation_handler* create(
      std::pair<void*, std::size_t> mem, ASIO_MOVE_ARG(H) handler)
  {
    if (sizeof(cancellation_handler) > mem.second)
    {
      ::operator delete(mem.first);
      mem.first = ::operator new(sizeof(cancellation_handler));
      mem.second = sizeof(cancellation_handler);
    }

    try
    {
      return new (mem.first) cancellation_handler(
          mem.second, ASIO_MOVE_CAST(H)(handler));
    }
    catch (...)
    {
      ::operator delete(mem.first);
      throw;
    }
  }

  void call()
  {
    handler_();
  }

  std::pair<void*, std::size_t> destroy() ASIO_NOEXCEPT
  {
    std::pair<void*, std::size_t> mem(this, size_);
    this->cancellation_handler::~cancellation_handler();
    return mem;
  }

private:
  template <typename H>
  cancellation_handler(std::size_t size, ASIO_MOVE_ARG(H) handler)
    : handler_(ASIO_MOVE_CAST(H)(handler)),
      size_(size)
  {
  }

  ~cancellation_handler()
  {
  }

  Handler handler_;
  std::size_t size_;
};

} // namespace detail

class cancellation_slot;

/// A cancellation signal with a single slot.
class cancellation_signal
{
public:
  cancellation_signal()
    : handler_(0)
  {
  }

  ~cancellation_signal()
  {
    if (handler_)
      ::operator delete(handler_->destroy().first);
  }

  /// Emits the signal and causes invocation of the slot's handler, if any.
  void emit()
  {
    if (handler_)
      handler_->call();
  }

  /// Returns the single slot associated with the signal.
  /**
   * The signal object must remain valid for as long the slot may be used.
   * Destruction of the signal invalidates the slot.
   */
  cancellation_slot slot();

private:
  cancellation_signal(const cancellation_signal&) ASIO_DELETED;
  cancellation_signal& operator=(const cancellation_signal&) ASIO_DELETED;

  detail::cancellation_handler_base* handler_;
};

/// A slot associated with a cancellation signal.
class cancellation_slot
{
public:
  /// Creates a slot that is not connected to any cancellation signal.
  ASIO_CONSTEXPR cancellation_slot()
    : handler_(0)
  {
  }

  /// Installs a handler into the slot.
  /**
   * Destroys any existing handler and context in the slot, then installs the
   * new handler with its associated context. The context is constructed with
   * @c args.
   *
   * @param handler A function object to be called when the signal is emitted.
   * The signature of the handler must be:
   * @code void handler(Context& context); @endcode
   *
   * @param args Arguments to be passed to the @c Context object's constructor.
   *
   * @returns A reference to the newly installed context.
   */
  template <typename Context, typename Handler, typename... Args>
  Context& emplace(ASIO_MOVE_ARG(Handler) handler,
      ASIO_MOVE_ARG(Args)... args)
  {
    assert(handler_);
    std::pair<void*, std::size_t> mem;
    if (*handler_)
    {
      mem = (*handler_)->destroy();
      *handler_ = 0;
    }
    detail::cancellation_handler<Context, Handler>* handler_obj
      = detail::cancellation_handler<Context, Handler>::create(
        mem, ASIO_MOVE_CAST(Handler)(handler),
        ASIO_MOVE_CAST(Args)(args)...);
    *handler_ = handler_obj;
    return handler_obj->context();
  }

  /// Installs a context-free handler into the slot.
  /**
   * Destroys any existing handler and context in the slot.
   *
   * @param handler A function object to be called when the signal is emitted.
   * The signature of the handler must be:
   * @code void handler(); @endcode
   */
  template <typename Handler>
  void emplace(ASIO_MOVE_ARG(Handler) handler)
  {
    assert(handler_);
    std::pair<void*, std::size_t> mem;
    if (*handler_)
    {
      mem = (*handler_)->destroy();
      *handler_ = 0;
    }
    detail::cancellation_handler<void, Handler>* handler_obj
      = detail::cancellation_handler<void, Handler>::create(
        mem, ASIO_MOVE_CAST(Handler)(handler));
    *handler_ = handler_obj;
  }

  /// Clears the slot.
  /**
   * Destroys any existing handler and associated context in the slot.
   */
  void clear()
  {
    if (handler_ != 0 && *handler_ != 0)
    {
      ::operator delete((*handler_)->destroy().first);
      *handler_ = 0;
    }
  }

  /// Returns whether the slot is connected to a signal.
  ASIO_CONSTEXPR bool is_connected() const ASIO_NOEXCEPT
  {
    return handler_ != 0;
  }

  /// Returns whether the slow is connected and has an installed handler.
  ASIO_CONSTEXPR bool has_handler() const ASIO_NOEXCEPT
  {
    return handler_ != 0 && *handler_ != 0;
  }

  /// Compare two slots for equality.
  friend ASIO_CONSTEXPR bool operator==(const cancellation_slot& lhs,
      const cancellation_slot& rhs) ASIO_NOEXCEPT
  {
    return lhs.handler_ == rhs.handler_;
  }

  /// Compare two slots for inequality.
  friend ASIO_CONSTEXPR bool operator!=(const cancellation_slot& lhs,
      const cancellation_slot& rhs) ASIO_NOEXCEPT
  {
    return lhs.handler_ != rhs.handler_;
  }

private:
  friend class cancellation_signal;

  ASIO_CONSTEXPR cancellation_slot(int,
      detail::cancellation_handler_base** handler)
    : handler_(handler)
  {
  }

  detail::cancellation_handler_base** handler_;
};

inline cancellation_slot cancellation_signal::slot()
{
  return cancellation_slot(0, &handler_);
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_CANCELLATION_SIGNAL_HPP
