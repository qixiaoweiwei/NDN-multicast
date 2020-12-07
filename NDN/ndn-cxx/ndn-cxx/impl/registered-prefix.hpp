/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2020 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#ifndef NDN_IMPL_REGISTERED_PREFIX_HPP
#define NDN_IMPL_REGISTERED_PREFIX_HPP

#include "ndn-cxx/impl/record-container.hpp"
#include "ndn-cxx/mgmt/nfd/command-options.hpp"

namespace ndn {

/**
 * @brief Stores information about a prefix registered in an NDN forwarder.
 */
class RegisteredPrefix : public detail::RecordBase<RegisteredPrefix>
{
public:
  RegisteredPrefix(const Name& prefix, const nfd::CommandOptions& options,
                   detail::RecordId filterId = 0)
    : m_prefix(prefix)
    , m_options(options)
    , m_filterId(filterId)
  {
  }

  const Name&
  getPrefix() const
  {
    return m_prefix;
  }

  const nfd::CommandOptions&
  getCommandOptions() const
  {
    return m_options;
  }

  detail::RecordId
  getFilterId() const
  {
    return m_filterId;
  }

private:
  Name m_prefix;
  nfd::CommandOptions m_options;
  detail::RecordId m_filterId;
};

} // namespace ndn

#endif // NDN_IMPL_REGISTERED_PREFIX_HPP
