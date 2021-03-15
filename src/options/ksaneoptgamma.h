/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2009-01-31
 * Description : Sane interface for KDE
 *
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * ============================================================ */

#ifndef KSANE_OPT_GAMMA_H
#define KSANE_OPT_GAMMA_H

#include "ksaneoption.h"

namespace KSaneIface
{

class KSaneOptGamma : public KSaneOption
{
    Q_OBJECT

public:
    KSaneOptGamma(const SANE_Handle handle, const int index);

    void readValue() override;
    void readOption() override;

    bool getValue(float &val) override;
    bool getValue(QString &val) override;
    bool getMaxValue(float &val) override;

public Q_SLOTS:
    bool setValue(const QVariant & value) override;

private:
    void calculateGTwriteData();
    
    int             m_brightness;
    int             m_contrast;
    int             m_gamma;
    QVector<int>    m_gammaTable;
};

}  // NameSpace KSaneIface

#endif
