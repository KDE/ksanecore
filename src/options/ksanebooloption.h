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

#ifndef KSANE_BOOL_OPTION_H
#define KSANE_BOOL_OPTION_H

#include "ksaneoption.h"

namespace KSaneIface
{

class KSaneBoolOption : public KSaneOption
{
    Q_OBJECT

public:
    KSaneBoolOption(const SANE_Handle handle, const int index);

    void readValue() override;

    QVariant getValue() const override;
    QString getValueAsString() const override;

public Q_SLOTS:
    bool setValue(const QVariant &value) override;

private:
    bool             m_checked;
};

}  // NameSpace KSaneIface

#endif // KSANE_BOOL_OPTION_H
