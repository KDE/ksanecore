/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2009-01-31
 * Description : Sane interface for KDE
 *
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * ============================================================ */

#include "ksaneoptgamma.h"

#include <QVarLengthArray>

#include <ksane_debug.h>

#include <cmath>

namespace KSaneIface
{

KSaneOptGamma::KSaneOptGamma(const SANE_Handle handle, const int index)
    : KSaneOption(handle, index)
{
    m_optionType = KSaneOption::TypeGamma;
}

bool KSaneOptGamma::setValue(const QVariant &value)
{
    if (state() == StateHidden) {
        return false;
    }
    
    if (static_cast<QMetaType::Type>(value.type()) == QMetaType::QString) {
        const QString stringValue = value.toString();
        QStringList gammaValues;
        int brightness;
        int contrast;
        int gamma;
        bool ok = true;

        gammaValues = stringValue.split(QLatin1Char(':'));
        if (gammaValues.size() != 3) {
            return false;
        }
        brightness = gammaValues.at(0).toInt(&ok);
        if (ok) {
            contrast = gammaValues.at(1).toInt(&ok);
        }
        if (ok) {
            gamma = gammaValues.at(2).toInt(&ok);
        }

        if (ok && (m_brightness != brightness || m_contrast != contrast || m_gamma != gamma) ) {
            m_brightness = brightness;
            m_contrast = contrast;
            m_gamma = gamma;
            calculateGTwriteData();
        }
        return true;
    }
    if (value.canConvert<QVector<int>>()) {
        QVector<int> copy = value.value<QVector<int>>();
        if (copy.size() != 3) {
            return false;
        }
        if (m_brightness != copy.at(0) || m_contrast != copy.at(1) || m_gamma != copy.at(2) ) {
            m_brightness = copy.at(0);
            m_contrast = copy.at(1);
            m_gamma = copy.at(2);
            calculateGTwriteData();
        }
        return true;
    }
    return false;
}

void KSaneOptGamma::readOption()
{
    KSaneOption::readOption();
    
    if (m_optDesc) {
        int size = m_optDesc->size / sizeof(SANE_Word);
        m_gammaTable.resize(size);
        for (int i = 0; i < m_gammaTable.size(); i++) {
            m_gammaTable[i] = i;
        }
    }
}

void KSaneOptGamma::readValue()
{
    // Unfortunately gamma table to brightness, contrast and gamma is
    // not easy nor fast.. ergo not done
}

bool KSaneOptGamma::getValue(float &)
{
    return false;
}

bool KSaneOptGamma::getMaxValue(float &value)
{
    if (m_optDesc) {
        value = static_cast<float>(m_optDesc->constraint.range->max);
        return true;
    }
    return false;
}

bool KSaneOptGamma::getValue(QString &val)
{
    if (state() == StateHidden) {
        return false;
    }

    val = QString::asprintf("%d:%d:%d", m_brightness, m_contrast, m_gamma);
    return true;
}

void KSaneOptGamma::calculateGTwriteData()
{   
    double maxValue = m_optDesc->constraint.range->max;
    double gamma    = 100.0 / m_gamma;
    double contrast = (200.0 / (100.0 - m_contrast)) - 1;
    double halfMax  = maxValue / 2.0;
    double brightness = (m_brightness / halfMax) * maxValue;
    double x;

    for (int i = 0; i < m_gammaTable.size(); i++) {
        // apply gamma
        x = std::pow(static_cast<double>(i) / m_gammaTable.size(), gamma) * maxValue;

        // apply contrast
        x = (contrast * (x - halfMax)) + halfMax;

        // apply brightness + rounding
        x += brightness + 0.5;

        // ensure correct value
        if (x > maxValue) {
            x = maxValue;
        }
        if (x < 0) {
            x = 0;
        }

        m_gammaTable[i] = static_cast<int>(x);
    }

    writeData(m_gammaTable.data());
    QVector<int> values = { m_brightness, m_contrast, m_gamma };
    Q_EMIT valueChanged(QVariant::fromValue(values));
}

}  // NameSpace KSaneIface
