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
    if (value.canConvert<QVariantList>()) { // It's a list
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

void GammaOption::readValue()
{
    if (state() == Option::StateHidden) {
        return;
    }

    QVarLengthArray<unsigned char> data(m_optDesc->size);
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option(m_handle, m_index, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        return;
    }

    QVector<int> gammaTable;
    gammaTable.reserve(data.size() / sizeof(int));
    for (int i = 0; i < data.size(); i += sizeof(SANE_Word)) gammaTable.append(toSANE_Word(&data[i]));

    if (gammaTable != m_gammaTable) {
        m_gammaTable = gammaTable;

        m_gammaTableMax = m_optDesc->constraint.range->max;

        calculateBCGwriteData();
    }
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
    // NOTE: This used to add the value times 2, not scaled to maxValue
    double brightness = m_brightness * maxValue / 100.0;
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

void GammaOption::calculateBCGwriteData() {
    int beginIndex = 0;
    int endIndex = m_gammaTable.size() - 1;
    // Find the start and end of the curve, to skip the flat regions
    while (beginIndex < endIndex && m_gammaTable[beginIndex] == m_gammaTable[0])
        beginIndex++;
    while (endIndex > beginIndex && m_gammaTable[endIndex] == m_gammaTable[m_gammaTable.size()-1])
        endIndex--;

    float gamma = 0, contrast = 0, brightness = 0;
    const QVector<int> &gammaTable = m_gammaTable;
    const int &gammaTableMax = m_gammaTableMax;

    auto guessGamma = [&gammaTable, &gamma](int i1, int i2, int step) {
        int diff1 = gammaTable[i1 + step] - gammaTable[i1 - step];
        int diff2 = gammaTable[i2 + step] - gammaTable[i2 - step];
        if (diff1 == 0 || diff2 == 0)
            return;
        float stepProportion = static_cast<float>(i2) / i1;
        float diffProportion = static_cast<float>(diff2) / diff1;
        float guessedGamma = log(stepProportion * diffProportion) / log(stepProportion);
        gamma += guessedGamma;
    };

    auto guessContrast = [&gammaTable, &gammaTableMax, &gamma, &contrast](int i1, int i2, int) {
        int prevVal = gammaTable[i1], nextVal = gammaTable[i2];
        float scaledDiff = static_cast<float>(nextVal - prevVal) / gammaTableMax;
        float scaledPrevIndex = static_cast<float>(i1) / gammaTable.size();
        float scaledNextIndex = static_cast<float>(i2) / gammaTable.size();
        float guessedContrast = scaledDiff / (pow(scaledNextIndex, gamma) - pow(scaledPrevIndex, gamma));
        contrast += guessedContrast;
    };

    auto guessBrightness = [&gammaTable, &gammaTableMax, &gamma, &contrast, &brightness](int i, int, int) {
        float scaledThisVal = static_cast<float>(gammaTable[i]) / gammaTableMax;
        float scaledIndex = static_cast<float>(i) / gammaTable.size();
        float guessedBrightness = scaledThisVal - ((pow(scaledIndex, gamma) - 0.5) * contrast + 0.5);
        brightness += guessedBrightness;
    };

    const int numberOfApproximations = 16;

    auto passValuePairsAndSteps = [&beginIndex, &endIndex](auto func) {
        const int step = (endIndex - beginIndex) / 8;
        for (int i = 0; i < numberOfApproximations;) {
            // Calculate step, even if not passed to the function, to separate the samples
            int i1 = rand() % (endIndex - beginIndex - 2 * step - 2) + beginIndex + step + 1;
            int i2 = rand() % (endIndex - beginIndex - 2 * step - 2) + beginIndex + step + 1;
            if (i2 - i1 >= 4 * step) {
                func(i1, i2, step);
                i++;
            }
        }
    };

    if (endIndex == beginIndex) {
        qCDebug(KSANECORE_LOG()) << "Ignoring gamma table: horizontal line at" << m_gammaTable[0];
        setValue(QVariantList{0, 0, 100}); // Ignore the table, it's wrong
        return;
    }

    if (endIndex - beginIndex <= 32) { // Table too small, make single guesses
        if (endIndex - beginIndex > 4) { // Measurements don't overlap by just one value
            guessGamma(beginIndex + 2, endIndex - 2, 2);
        } else {
            gamma = 1.0; // Assume linear gamma
        }
        guessContrast(beginIndex, endIndex, 0);
        guessBrightness((beginIndex + endIndex) / 2, 0, 0);
    } else {
        passValuePairsAndSteps(guessGamma);
        gamma /= numberOfApproximations;

        passValuePairsAndSteps(guessContrast);
        contrast /= numberOfApproximations;

        passValuePairsAndSteps(guessBrightness);
        brightness /= numberOfApproximations;
    }

    int newGamma = 100.0 / gamma;
    int newContrast = 100.0 - 200.0 / (contrast + 1.0);
    int newBrightness = brightness * 100.0;

    if (m_gamma != newGamma || m_contrast != newContrast || m_brightness != newBrightness) {
        m_gamma = newGamma;
        m_contrast = newContrast;
        m_brightness = newBrightness;

        QVariantList values = { m_brightness, m_contrast, m_gamma };
        Q_EMIT valueChanged(values);
    }
}

} // namespace KSaneCore

#include "moc_gammaoption.cpp"
