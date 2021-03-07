/* ============================================================
 *
 * This file is part of the KDE project
 *
 * Date        : 2009-01-21
 * Description : Sane interface for KDE
 *
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * ============================================================ */

#include "ksaneoptcombo.h"

#include "labeledcombo.h"

#include <QtCore/QVarLengthArray>

#include <QIcon>
#include <QLocale>

#include <ksane_debug.h>

namespace KSaneIface
{
static const char tmp_binary[] = "Binary";

KSaneOptCombo::KSaneOptCombo(const SANE_Handle handle, const int index)
    : KSaneOption(handle, index), m_combo(nullptr)
{
}

void KSaneOptCombo::createWidget(QWidget *parent)
{
    if (m_widget) {
        return;
    }

    m_widget = m_combo = new LabeledCombo(parent, QString(), QStringList());
    readOption();
    m_widget->setToolTip(sane_i18n(m_optDesc->desc));
    connect(m_combo, QOverload<int>::of(&LabeledCombo::activated), this, &KSaneOptCombo::comboboxChangedIndex);
    readValue();
}

void KSaneOptCombo::readValue()
{
    if (state() == STATE_HIDDEN) {
        return;
    }

    // read that current value
    QVarLengthArray<unsigned char> data(m_optDesc->size);
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option(m_handle, m_index, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        return;
    }

    const std::pair<QString, QString> current = getSaneComboString(data.data());
    m_currentText = current.second;
    if (m_combo != nullptr) {
        if (m_combo->currentData() != current.first) {
            m_combo->setCurrentText(m_currentText);
            Q_EMIT valueChanged();
        }
    }
}

void KSaneOptCombo::readOption()
{
    KSaneOption::readOption();

    if (!m_combo) {
        return;
    }

    QString saved = m_combo->currentText();

    QList<std::pair<QString, QString>> list = genComboStringList();
    m_combo->clear();
    m_combo->setLabelText(sane_i18n(m_optDesc->title));
    for (int i = 0; i < list.count(); ++i) {
        m_combo->addItem(list[i].second, list[i].first);
    }
    m_combo->setIcon(QIcon::fromTheme(QStringLiteral("color")),
                     getSaneComboString((unsigned char *)SANE_VALUE_SCAN_MODE_COLOR).second);
    m_combo->setIcon(QIcon::fromTheme(QStringLiteral("gray-scale")),
                     getSaneComboString((unsigned char *)SANE_VALUE_SCAN_MODE_GRAY).second);
    m_combo->setIcon(QIcon::fromTheme(QStringLiteral("black-white")),
                     getSaneComboString((unsigned char *)SANE_VALUE_SCAN_MODE_LINEART).second);
    // The epkowa/epson backend uses "Binary" which is the same as "Lineart"
    m_combo->setIcon(QIcon::fromTheme(QStringLiteral("black-white")), i18n(tmp_binary));

    // set the previous value
    m_combo->setCurrentText(saved);
}

QList<std::pair<QString, QString>> KSaneOptCombo::genComboStringList() const
{
    int i;
    QList<std::pair<QString, QString>> list;

    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
        for (i = 1; i <= m_optDesc->constraint.word_list[0]; ++i) {
            const QString tmp = getSaneComboString((int)m_optDesc->constraint.word_list[i]);
            list += std::make_pair(tmp, tmp);
        }
        break;
    case SANE_TYPE_FIXED:
        for (i = 1; i <= m_optDesc->constraint.word_list[0]; ++i) {
            const QString tmp = getSaneComboString((float)SANE_UNFIX(m_optDesc->constraint.word_list[i]));
            list += std::make_pair(tmp, tmp);
        }
        break;
    case SANE_TYPE_STRING:
        i = 0;
        while (m_optDesc->constraint.string_list[i] != nullptr) {
            list += getSaneComboString((unsigned char *)m_optDesc->constraint.string_list[i]);
            i++;
        }
        break;
    default :
        list += std::make_pair(QStringLiteral("NOT HANDELED"), QStringLiteral("NOT HANDELED"));
        break;
    }
    return list;
}

QString KSaneOptCombo::getSaneComboString(int ival) const
{
    switch (m_optDesc->unit) {
    case SANE_UNIT_NONE:        break;
    case SANE_UNIT_PIXEL:       return i18np("%1 Pixel", "%1 Pixels", ival);
    case SANE_UNIT_BIT:         return i18np("%1 Bit", "%1 Bits", ival);
    case SANE_UNIT_MM:          return i18np("%1 mm", "%1 mm", ival);
    case SANE_UNIT_DPI:         return i18np("%1 DPI", "%1 DPI", ival);
    case SANE_UNIT_PERCENT:     return i18np("%1 %", "%1 %", ival);
    case SANE_UNIT_MICROSECOND: return i18np("%1 µs", "%1 µs", ival);
    }
    return QString::number(ival);
}

QString KSaneOptCombo::getSaneComboString(float fval) const
{
    switch (m_optDesc->unit) {
    case SANE_UNIT_NONE:        break;
    case SANE_UNIT_PIXEL:       return i18ncp("Parameter and Unit", "%1 Pixel", "%1 Pixels", fval);
    case SANE_UNIT_BIT:         return i18ncp("Parameter and Unit", "%1 Bit", "%1 Bits", fval);
    case SANE_UNIT_MM:          return i18nc("Parameter and Unit (Millimeter)", "%1 mm", fval);
    case SANE_UNIT_DPI:         return i18nc("Parameter and Unit (Dots Per Inch)", "%1 DPI", fval);
    case SANE_UNIT_PERCENT:     return i18nc("Parameter and Unit (Percentage)", "%1 %", fval);
    case SANE_UNIT_MICROSECOND: return i18nc("Parameter and Unit (Microseconds)", "%1 µs", fval);
    }
    return QString::number(fval, 'F', 4);
}

std::pair<QString, QString> KSaneOptCombo::getSaneComboString(unsigned char *data) const
{
    QString tmp;
    if (data == nullptr) {
        return std::pair<QString, QString>();
    }

    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
        tmp = getSaneComboString((int)toSANE_Word(data));
        return std::make_pair(tmp, tmp);
    case SANE_TYPE_FIXED:
        tmp = getSaneComboString((float)SANE_UNFIX(toSANE_Word(data)));
        return std::make_pair(tmp, tmp);
    case SANE_TYPE_STRING:
        return std::make_pair(QString::fromUtf8(reinterpret_cast<char *>(data)), sane_i18n(reinterpret_cast<char *>(data)));
    default :
        break;
    }
    return std::pair<QString, QString>();
}

void KSaneOptCombo::comboboxChangedIndex(int i)
{
    if (m_combo && (m_combo->currentText() == m_currentText)) {
        return;
    }

    unsigned char data[4];
    void *dataPtr;

    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
    case SANE_TYPE_FIXED:
        fromSANE_Word(data, m_optDesc->constraint.word_list[i + 1]);
        dataPtr = data;
        break;
    case SANE_TYPE_STRING:
        dataPtr = (void *)m_optDesc->constraint.string_list[i];
        break;
    default:
        qCDebug(KSANE_LOG) << "can not handle type:" << m_optDesc->type;
        return;
    }
    writeData(dataPtr);
    readValue();
    Q_EMIT valueChanged();
}

bool KSaneOptCombo::getMinValue(float &val)
{
    if (state() == STATE_HIDDEN) {
        return false;
    }
    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
        val = (float)m_optDesc->constraint.word_list[1];
        for (int i = 2; i <= m_optDesc->constraint.word_list[0]; i++) {
            val = qMin((float)m_optDesc->constraint.word_list[i], val);
        }
        break;
    case SANE_TYPE_FIXED:
        val = (float)SANE_UNFIX(m_optDesc->constraint.word_list[1]);
        for (int i = 2; i <= m_optDesc->constraint.word_list[0]; i++) {
            val = qMin((float)SANE_UNFIX(m_optDesc->constraint.word_list[i]), val);
        }
        break;
    default:
        qCDebug(KSANE_LOG) << "can not handle type:" << m_optDesc->type;
        return false;
    }
    return true;
}

bool KSaneOptCombo::getValue(float &val)
{
    if (state() == STATE_HIDDEN) {
        return false;
    }

    // read that current value
    QVarLengthArray<unsigned char> data(m_optDesc->size);
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option(m_handle, m_index, SANE_ACTION_GET_VALUE, data.data(), &res);
    if (status != SANE_STATUS_GOOD) {
        qCDebug(KSANE_LOG) << m_optDesc->name << "sane_control_option returned" << status;
        return false;
    }

    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
        val = (float)toSANE_Word(data.data());
        return true;
    case SANE_TYPE_FIXED:
        val = SANE_UNFIX(toSANE_Word(data.data()));
        return true;
    default:
        qCDebug(KSANE_LOG) << "Type" << m_optDesc->type << "not supported!";
        break;
    }
    return false;
}

bool KSaneOptCombo::setValue(float value)
{
    unsigned char data[4];
    float tmp;
    float minDiff;
    int i;
    int minIndex = 1;

    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
        tmp = (float)m_optDesc->constraint.word_list[minIndex];
        minDiff = qAbs(value - tmp);
        for (i = 2; i <= m_optDesc->constraint.word_list[0]; ++i) {
            tmp = (float)m_optDesc->constraint.word_list[i];
            if (qAbs(value - tmp) < minDiff) {
                minDiff = qAbs(value - tmp);
                minIndex = i;
            }
        }
        fromSANE_Word(data, m_optDesc->constraint.word_list[minIndex]);
        writeData(data);
        readValue();
        return (minDiff < 1.0);
    case SANE_TYPE_FIXED:
        tmp = (float)SANE_UNFIX(m_optDesc->constraint.word_list[minIndex]);
        minDiff = qAbs(value - tmp);
        for (i = 2; i <= m_optDesc->constraint.word_list[0]; ++i) {
            tmp = (float)SANE_UNFIX(m_optDesc->constraint.word_list[i]);
            if (qAbs(value - tmp) < minDiff) {
                minDiff = qAbs(value - tmp);
                minIndex = i;
            }
        }
        fromSANE_Word(data, m_optDesc->constraint.word_list[minIndex]);
        writeData(data);
        readValue();
        return (minDiff < 1.0);
    default:
        qCDebug(KSANE_LOG) << "can not handle type:" << m_optDesc->type;
        break;
    }
    return false;
}

bool KSaneOptCombo::getValue(QString &val)
{
    if (state() == STATE_HIDDEN) {
        return false;
    }
    val = m_combo->currentData().toString();
    return true;
}

bool KSaneOptCombo::setValue(const QString &val)
{
    if (state() == STATE_HIDDEN) {
        return false;
    }

    unsigned char data[4];
    void* data_ptr = nullptr;
    SANE_Word fixed;
    int i;
    float f;
    bool ok;
    QString tmp;

    switch (m_optDesc->type) {
    case SANE_TYPE_INT:
        tmp = val.left(val.indexOf(QLatin1Char(' '))); // strip the unit
        // accept float formatting of the string
        i = (int)(QLocale::system().toFloat(tmp,&ok));
        if (ok == false) {
            return false;
        }
        fromSANE_Word(data, i);
        data_ptr = data;
        break;
    case SANE_TYPE_FIXED:
        tmp = val.left(val.indexOf(QLatin1Char(' '))); // strip the unit
        f = QLocale::system().toFloat(tmp,&ok);
        if (ok == false) {
            return false;
        }
        fixed = SANE_FIX(f);
        fromSANE_Word(data, fixed);
        data_ptr = data;
        break;
    case SANE_TYPE_STRING:
        i = 0;
        while (m_optDesc->constraint.string_list[i] != nullptr) {
            tmp = getSaneComboString((unsigned char *)m_optDesc->constraint.string_list[i]).first;
            if (val == tmp) {
                data_ptr = (void *)m_optDesc->constraint.string_list[i];
                break;
            }
            i++;
        }
        if (m_optDesc->constraint.string_list[i] == nullptr) {
            return false;
        }
        break;
    default:
        qCDebug(KSANE_LOG) << "can only handle SANE_TYPE: INT, FIXED and STRING";
        return false;
    }
    writeData(data_ptr);

    readValue();
    return true;
}

bool KSaneOptCombo::hasGui()
{
    return true;
}

}  // NameSpace KSaneIface
