/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2009-01-21
 * Description : Sane interface for KDE
 *
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * ============================================================ */

#ifndef KSANE_STRING_OPTION_H
#define KSANE_STRING_OPTION_H

#include "ksaneoption.h"

namespace KSaneIface
{

class KSaneStringOption : public KSaneOption
{
    Q_OBJECT

public:
    KSaneStringOption(const SANE_Handle handle, const int index);

    void readValue() override;

    QVariant getValue() const override;
    QString getValueAsString() const override;

public Q_SLOTS:
    bool setValue(const QVariant &val) override;

private:
    QString       m_string;
};

}  // NameSpace KSaneIface

#endif // KSANE_STRING_OPTION_H
