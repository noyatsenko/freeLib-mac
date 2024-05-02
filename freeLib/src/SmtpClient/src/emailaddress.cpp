/*
  Copyright (c) 2011-2012 - Tőkés Attila

  This file is part of SmtpClient for Qt.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  See the LICENSE file for more details.
*/

#include "emailaddress.h"

/*!
    \class EmailAddress
    \brief Represent an e-mail address and display name.

    EmailAddress is used to specify both sender and reicipents of
    e-mail messages.
    \sa SmtpClient
*/

EmailAddress::EmailAddress(const QString & address, const QString & name)
    : address(address), name(name)
{
}

EmailAddress::EmailAddress(const EmailAddress &other)
    : address(other.address), name(other.name)
{
}

EmailAddress::EmailAddress(EmailAddress&& other)
    : address(std::move(other.address)), name(std::move(other.name))
{
}

/*!
    Returns the user friendly name for this address. Usually this is the 
    person's name.
*/
QString EmailAddress::getName() const
{
    return name;
}

/*!
    Returns the e-mail address in the form user@domain
*/
QString EmailAddress::getAddress() const
{
    return address;
}

EmailAddress& EmailAddress::operator=(const EmailAddress& other) {
    address = other.address;
    name = other.name;
    return *this;
}

EmailAddress& EmailAddress::operator=(EmailAddress&& other) {
    address = std::move(other.address);
    name = std::move(other.name);
    return *this;
}