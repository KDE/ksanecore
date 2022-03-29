/*
 * SPDX-FileCopyrightText: 2009 Kare Sars <kare dot sars at iki dot fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "stringoption.h"

#include <QVarLengthArray>

#include <ksanecore_debug.h>

namespace KSane
{

StringOption::StringOption(const SANE_Handle handle, const int index)
    : BaseOption(handle, index)
{
    m_optionType = CoreOption::TypeString;
}

bool StringOption::setValue(const QVariant &val)
{
    if (state() == CoreOption::StateHidden) {
        return false;
    }
    QString text = val.toString();
    QString tmp;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    tmp += QStringView(text).left(m_optDesc->size);
#else
    tmp += text.leftRef(m_optDesc->size);
#endif
    if (tmp != text) {
        writeData(tmp.toLatin1().data());
        Q_EMIT valueChanged(tmp);
    }
    return true;
}

void StringOption::readValue()
{
    if (state() == CoreOption::StateHidden) {
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

    m_string = QString::fromUtf8(reinterpret_cast<char *>(data.data()));

    Q_EMIT valueChanged(m_string);
}

QVariant StringOption::value() const
{
    return QVariant(m_string);
}

int StringOption::valueSize() const
{
    return static_cast<int>(m_optDesc->size);
}

QString StringOption::valueAsString() const
{
    if (state() == CoreOption::StateHidden) {
        return QString();
    }
    return m_string;
}

} // namespace KSane
