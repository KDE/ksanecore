/*
 * SPDX-FileCopyrightText: 2010 Kare Sars <kare dot sars at iki dot fi>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef KSANE_AUTHENTICATION_H
#define KSANE_AUTHENTICATION_H

// Qt includes
#include <QString>

// Sane includes
extern "C"
{
#include <sane/saneopts.h>
#include <sane/sane.h>
}

namespace KSaneCore
{

/**
 * Sane authentication helpers.
 */
class Authentication
{
public:
    static Authentication *getInstance();
    ~Authentication();

    void setDeviceAuth(const QString &resource, const QString &username, const QString &password);
    void clearDeviceAuth(const QString &resource);
    static void authorization(SANE_String_Const resource, SANE_Char *username, SANE_Char *password);

private:
    Authentication();
    struct Private;
    Private *const d;
};

} // namespace KSaneCore

#endif // KSANE_AUTHENTICATION_H
