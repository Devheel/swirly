/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2018 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef SWIRLY_WEB_HTTPRESPONSE_HPP
#define SWIRLY_WEB_HTTPRESPONSE_HPP

#include <swirly/util/Stream.hpp>

#include <swirly/sys/Buffer.hpp>

namespace swirly {

class SWIRLY_API HttpResponseBuf : public std::streambuf {
  public:
    explicit HttpResponseBuf(Buffer& buf) noexcept
      : buf_(buf)
    {
    }
    ~HttpResponseBuf() noexcept override;

    // Copy.
    HttpResponseBuf(const HttpResponseBuf& rhs) = delete;
    HttpResponseBuf& operator=(const HttpResponseBuf& rhs) = delete;

    // Move.
    HttpResponseBuf(HttpResponseBuf&&) = delete;
    HttpResponseBuf& operator=(HttpResponseBuf&&) = delete;

    std::streamsize pcount() const noexcept { return pcount_; }
    void commit() noexcept { buf_.commit(pcount_); }
    void reset() noexcept
    {
        pbase_ = nullptr;
        pcount_ = 0;
    }
    void setContentLength(std::streamsize pos, std::streamsize len) noexcept;

  protected:
    int_type overflow(int_type c) noexcept override;

    std::streamsize xsputn(const char_type* s, std::streamsize count) noexcept override;

  private:
    Buffer& buf_;
    char* pbase_{nullptr};
    std::streamsize pcount_{0};
};

class SWIRLY_API HttpResponse : public std::ostream {
  public:
    explicit HttpResponse(Buffer& buf) noexcept
      : std::ostream{nullptr}
      , buf_{buf}
    {
        rdbuf(&buf_);
    }
    ~HttpResponse() noexcept override;

    // Copy.
    HttpResponse(const HttpResponse& rhs) = delete;
    HttpResponse& operator=(const HttpResponse& rhs) = delete;

    // Move.
    HttpResponse(HttpResponse&&) = delete;
    HttpResponse& operator=(HttpResponse&&) = delete;

    std::streamsize pcount() const noexcept { return buf_.pcount(); }
    void commit() noexcept;
    void reset() noexcept
    {
        buf_.reset();
        swirly::reset(*this);
        cloff_ = hcount_ = 0;
    }
    void reset(int status, const char* reason, bool cache = false);

  private:
    HttpResponseBuf buf_;
    /**
     * Content-Length offset.
     */
    std::streamsize cloff_{0};
    /**
     * Header size.
     */
    std::streamsize hcount_{0};
};

} // namespace swirly

#endif // SWIRLY_WEB_HTTPRESPONSE_HPP
