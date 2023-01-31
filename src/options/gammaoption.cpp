/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "gammaoption.h"

#include <QVarLengthArray>

#include <ksanecore_debug.h>

#include <cmath>

namespace KSaneCore
{

GammaOption::GammaOption(const SANE_Handle handle, const int index)
    : BaseOption(handle, index)
{
    m_optionType = Option::TypeGamma;
}

bool GammaOption::setValue(const QVariant &value)
{
    if (state() == Option::StateHidden) {
        return false;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (static_cast<QMetaType::Type>(value.type()) == QMetaType::QString) {
#else
    if (value.userType() == QMetaType::QString) {
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (static_cast<QMetaType::Type>(value.type()) == QMetaType::QVariantList) {
#else
    if (value.userType() == QMetaType::QVariantList) {
#endif
        QVariantList copy = value.toList();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (copy.size() != 3 || static_cast<QMetaType::Type>(copy.at(0).type()) != QMetaType::Int
            || static_cast<QMetaType::Type>(copy.at(1).type()) != QMetaType::Int || static_cast<QMetaType::Type>(copy.at(2).type()) != QMetaType::Int) {
#else
        if (copy.size() != 3 || copy.at(0).userType() != QMetaType::Int || copy.at(1).userType() != QMetaType::Int || copy.at(2).userType() != QMetaType::Int) {
#endif
            return false;
        }
        if (m_brightness != copy.at(0).toInt() || m_contrast != copy.at(1).toInt() || m_gamma != copy.at(2).toInt() ) {
            m_brightness = copy.at(0).toInt();
            m_contrast = copy.at(1).toInt();
            m_gamma = copy.at(2).toInt();
            calculateGTwriteData();
        }
        return true;
    }
    return false;
}

void GammaOption::readOption()
{
    beginOptionReload();

    if (m_optDesc) {
        int size = m_optDesc->size / sizeof(SANE_Word);
        m_gammaTable.resize(size);
        for (int i = 0; i < m_gammaTable.size(); i++) {
            m_gammaTable[i] = i;
        }
    }

    endOptionReload();
}

void GammaOption::readValue()
{
    // Unfortunately gamma table to brightness, contrast and gamma is
    // not easy nor fast.. ergo not done
}

QVariant GammaOption::value() const
{
    if (state() == Option::StateHidden) {
        return QVariant();
    }
    return QVariantList{ m_brightness, m_contrast, m_gamma };
}

int GammaOption::valueSize() const
{
    return 3;
}

QVariant GammaOption::maximumValue() const
{
    QVariant value;
    if (m_optDesc) {
        value = static_cast<float>(m_optDesc->constraint.range->max);
        return value;
    }
    return value;
}

QString GammaOption::valueAsString() const
{
    if (state() == Option::StateHidden) {
        return QString();
    }

    return QString::asprintf("%d:%d:%d", m_brightness, m_contrast, m_gamma);
}

void GammaOption::calculateGTwriteData()
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
    QVariantList values = { m_brightness, m_contrast, m_gamma };
    Q_EMIT valueChanged(values);
}

} // namespace KSaneCore
