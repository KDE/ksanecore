/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "baseoption.h"

#include <ksanecore_debug.h>

namespace KSane
{

BaseOption::BaseOption() : QObject()
{
}

BaseOption::BaseOption(const SANE_Handle handle, const int index)
    : QObject(), m_handle(handle), m_index(index)
{
}

BaseOption::~BaseOption()
{
    if (m_data) {
        free(m_data);
        m_data = nullptr;
    }
}

void BaseOption::readOption()
{
    if (m_handle != nullptr) {
        m_optDesc = sane_get_option_descriptor(m_handle, m_index);
        Q_EMIT optionReloaded();
    }
}

CoreOption::OptionState BaseOption::state() const
{
    if (m_optDesc == nullptr) {
        return CoreOption::StateHidden;
    }

    if (((m_optDesc->cap & SANE_CAP_SOFT_DETECT) == 0) ||
            (m_optDesc->cap & SANE_CAP_INACTIVE) ||
            ((m_optDesc->size == 0) && (type() != CoreOption::TypeAction))) {
        return CoreOption::StateHidden;
    } else if ((m_optDesc->cap & SANE_CAP_SOFT_SELECT) == 0) {
        return CoreOption::StateDisabled;
    }
    return CoreOption::StateActive;
}

bool BaseOption::needsPolling() const
{
    if (!m_optDesc) {
        return false;
    }

    if ((m_optDesc->cap & SANE_CAP_SOFT_DETECT) && !(m_optDesc->cap & SANE_CAP_SOFT_SELECT)) {
        qCDebug(KSANECORE_LOG) << name() << "optDesc->cap =" << m_optDesc->cap;
        return true;
    }

    return false;
}

QString BaseOption::name() const
{
    if (m_optDesc == nullptr) {
        return QString();
    }
    return QString::fromUtf8(m_optDesc->name);
}

QString BaseOption::title() const
{
    if (m_optDesc == nullptr) {
        return QString();
    }
    return sane_i18n(m_optDesc->title);
}

QString BaseOption::description() const
{
    if (m_optDesc == nullptr) {
        return QString();
    }
    return sane_i18n(m_optDesc->desc);
}

CoreOption::OptionType BaseOption::type() const
{
    return m_optionType;
}

bool BaseOption::writeData(void *data)
{
    SANE_Status status;
    SANE_Int res;

    if (state() == CoreOption::StateDisabled) {
        return false;
    }

    status = sane_control_option(m_handle, m_index, SANE_ACTION_SET_VALUE, data, &res);
    if (status != SANE_STATUS_GOOD) {
        qCDebug(KSANECORE_LOG) << m_optDesc->name << "sane_control_option returned:" << sane_strstatus(status);
        // write failed. re read the current setting
        readValue();
        return false;
    }
    if (res & SANE_INFO_INEXACT) {
        //qCDebug(KSANECORE_LOG) << "write was inexact. Reload value just in case...";
        readValue();
    }

    if (res & SANE_INFO_RELOAD_OPTIONS) {
        Q_EMIT optionsNeedReload();
        // optReload reloads also the values
    } else if (res & SANE_INFO_RELOAD_PARAMS) {
        // 'else if' because with optReload we force also valReload :)
        Q_EMIT valuesNeedReload();
    }

    return true;
}

void BaseOption::readValue() {}

SANE_Word BaseOption::toSANE_Word(unsigned char *data)
{
    SANE_Word tmp;
    // if __BYTE_ORDER is not defined we get #if 0 == 0
#if __BYTE_ORDER == __LITTLE_ENDIAN
    tmp  = (data[0] & 0xff);
    tmp += ((SANE_Word)(data[1] & 0xff)) << 8;
    tmp += ((SANE_Word)(data[2] & 0xff)) << 16;
    tmp += ((SANE_Word)(data[3] & 0xff)) << 24;
#else
    tmp  = (data[3] & 0xff);
    tmp += ((SANE_Word)(data[2] & 0xff)) << 8;
    tmp += ((SANE_Word)(data[1] & 0xff)) << 16;
    tmp += ((SANE_Word)(data[0] & 0xff)) << 24;
#endif
    return tmp;
}

void BaseOption::fromSANE_Word(unsigned char *data, SANE_Word from)
{
    // if __BYTE_ORDER is not defined we get #if 0 == 0
#if __BYTE_ORDER == __LITTLE_ENDIAN
    data[0] = (from & 0x000000FF);
    data[1] = (from & 0x0000FF00) >> 8;
    data[2] = (from & 0x00FF0000) >> 16;
    data[3] = (from & 0xFF000000) >> 24;
#else
    data[3] = (from & 0x000000FF);
    data[2] = (from & 0x0000FF00) >> 8;
    data[1] = (from & 0x00FF0000) >> 16;
    data[0] = (from & 0xFF000000) >> 24;
#endif
}

QVariant BaseOption::value() const
{
    return QVariant();
}

QVariant BaseOption::minimumValue() const
{
    return QVariant();
}

QVariant BaseOption::maximumValue() const
{
    return QVariant();
}

QVariant BaseOption::stepValue() const
{
    return QVariant();
}

QVariantList BaseOption::valueList() const
{
    return QVariantList();
}

QVariantList BaseOption::internalValueList() const
{
    return QVariantList();
}

CoreOption::OptionUnit BaseOption::valueUnit() const
{
    if (m_optDesc != nullptr) {
        switch (m_optDesc->unit) {
        case SANE_UNIT_PIXEL:
            return CoreOption::UnitPixel;
        case SANE_UNIT_BIT:
            return CoreOption::UnitBit;
        case SANE_UNIT_MM:
            return CoreOption::UnitMilliMeter;
        case SANE_UNIT_DPI:
            return CoreOption::UnitDPI;
        case SANE_UNIT_PERCENT:
            return CoreOption::UnitPercent;
        case SANE_UNIT_MICROSECOND:
            return CoreOption::UnitMicroSecond;
        default:
            return CoreOption::UnitNone;
        }
    } else {
        return CoreOption::UnitNone;
    }
}

int BaseOption::valueSize() const
{
    if (m_optDesc != nullptr) {
        return m_optDesc->size / sizeof(SANE_Word);
    }
    return 0;
}

QString BaseOption::valueAsString() const
{
    return QString();
}

bool BaseOption::setValue(const QVariant &)
{
    return false;
}

bool BaseOption::storeCurrentData()
{
    SANE_Status status;
    SANE_Int res;

    // check if we can read the value
    if (state() == CoreOption::StateHidden) {
        return false;
    }

    // read that current value
    if (m_data != nullptr) {
        free(m_data);
    }
    m_data = (unsigned char *)malloc(m_optDesc->size);
    status = sane_control_option(m_handle, m_index, SANE_ACTION_GET_VALUE, m_data, &res);
    if (status != SANE_STATUS_GOOD) {
        qCDebug(KSANECORE_LOG) << m_optDesc->name << "sane_control_option returned" << status;
        return false;
    }
    return true;
}

bool BaseOption::restoreSavedData()
{
    // check if we have saved any data
    if (m_data == nullptr) {
        return false;
    }

    // check if we can write the value
    if (state() == CoreOption::StateHidden) {
        return false;
    }
    if (state() == CoreOption::StateDisabled) {
        return false;
    }

    writeData(m_data);
    readValue();
    return true;
}

CoreOption::OptionType BaseOption::optionType(const SANE_Option_Descriptor *optDesc)
{
    if (!optDesc) {
        return CoreOption::TypeDetectFail;
    }

    switch (optDesc->constraint_type) {
    case SANE_CONSTRAINT_NONE:
        switch (optDesc->type) {
        case SANE_TYPE_BOOL:
            return CoreOption::TypeBool;
        case SANE_TYPE_INT:
            if (optDesc->size == sizeof(SANE_Word)) {
                return CoreOption::TypeInteger;
            }
            qCDebug(KSANECORE_LOG) << "Can not handle:" << optDesc->title;
            qCDebug(KSANECORE_LOG) << "SANE_CONSTRAINT_NONE && SANE_TYPE_INT";
            qCDebug(KSANECORE_LOG) << "size" << optDesc->size << "!= sizeof(SANE_Word)";
            break;
        case SANE_TYPE_FIXED:
            if (optDesc->size == sizeof(SANE_Word)) {
                return CoreOption::TypeDouble;
            }
            qCDebug(KSANECORE_LOG) << "Can not handle:" << optDesc->title;
            qCDebug(KSANECORE_LOG) << "SANE_CONSTRAINT_NONE && SANE_TYPE_FIXED";
            qCDebug(KSANECORE_LOG) << "size" << optDesc->size << "!= sizeof(SANE_Word)";
            break;
        case SANE_TYPE_BUTTON:
            return CoreOption::TypeAction;
        case SANE_TYPE_STRING:
            return CoreOption::TypeString;
        case SANE_TYPE_GROUP:
            return CoreOption::TypeDetectFail;
        }
        break;
    case SANE_CONSTRAINT_RANGE:
        switch (optDesc->type) {
        case SANE_TYPE_BOOL:
            return CoreOption::TypeBool;
        case SANE_TYPE_INT:
            if (optDesc->size == sizeof(SANE_Word)) {
                return CoreOption::TypeInteger;
            }

            if ((strcmp(optDesc->name, SANE_NAME_GAMMA_VECTOR) == 0) ||
                    (strcmp(optDesc->name, SANE_NAME_GAMMA_VECTOR_R) == 0) ||
                    (strcmp(optDesc->name, SANE_NAME_GAMMA_VECTOR_G) == 0) ||
                    (strcmp(optDesc->name, SANE_NAME_GAMMA_VECTOR_B) == 0)) {
                return CoreOption::TypeGamma;
            }
            qCDebug(KSANECORE_LOG) << "Can not handle:" << optDesc->title;
            qCDebug(KSANECORE_LOG) << "SANE_CONSTRAINT_RANGE && SANE_TYPE_INT && !SANE_NAME_GAMMA_VECTOR...";
            qCDebug(KSANECORE_LOG) << "size" << optDesc->size << "!= sizeof(SANE_Word)";
            break;
        case SANE_TYPE_FIXED:
            if (optDesc->size == sizeof(SANE_Word)) {
                return CoreOption::TypeDouble;
            }
            qCDebug(KSANECORE_LOG) << "Can not handle:" << optDesc->title;
            qCDebug(KSANECORE_LOG) << "SANE_CONSTRAINT_RANGE && SANE_TYPE_FIXED";
            qCDebug(KSANECORE_LOG) << "size" << optDesc->size << "!= sizeof(SANE_Word)";
            qCDebug(KSANECORE_LOG) << "Analog Gamma vector?";
            break;
        case SANE_TYPE_STRING:
            qCDebug(KSANECORE_LOG) << "Can not handle:" << optDesc->title;
            qCDebug(KSANECORE_LOG) << "SANE_CONSTRAINT_RANGE && SANE_TYPE_STRING";
            return CoreOption::TypeDetectFail;
        case SANE_TYPE_BUTTON:
            return CoreOption::TypeAction;
        case SANE_TYPE_GROUP:
            return CoreOption::TypeDetectFail;
        }
        break;
    case SANE_CONSTRAINT_WORD_LIST:
    case SANE_CONSTRAINT_STRING_LIST:
        return CoreOption::TypeValueList;
    }
    return CoreOption::TypeDetectFail;
}

} // namespace KSane
