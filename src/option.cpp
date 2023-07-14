/*
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "option.h"
#include "baseoption.h"
#include "option_p.h"

#include <ksanecore_debug.h>

namespace KSaneCore
{

Option::Option(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<OptionPrivate>())
{
}

Option::~Option() = default;

Option::OptionState Option::state() const
{
    if (d->option != nullptr) {
        return d->option->state();
    } else {
        return StateDisabled;
    }
}

QString Option::name() const
{
    if (d->option != nullptr) {
        return d->option->name();
    } else {
        return QString();
    }
}

QString Option::title() const
{
    if (d->option != nullptr) {
        return d->option->title();
    } else {
        return QString();
    }
}

QString Option::description() const
{
    if (d->option != nullptr) {
        return d->option->description();
    } else {
        return QString();
    }
}

Option::OptionType Option::type() const
{
    if (d->option != nullptr) {
        return d->option->type();
    } else {
        return TypeDetectFail;
    }
}

QVariant Option::minimumValue() const
{
    if (d->option != nullptr) {
        return d->option->minimumValue();
    } else {
        return QVariant();
    }
}

QVariant Option::maximumValue() const
{
    if (d->option != nullptr) {
        return d->option->maximumValue();
    } else {
        return QVariant();
    }
}

QVariant Option::stepValue() const
{
    if (d->option != nullptr) {
        return d->option->stepValue();
    } else {
        return QVariant();
    }
}

QVariantList Option::valueList() const
{
    if (d->option != nullptr) {
        return d->option->valueList();
    } else {
        return QVariantList();
    }
}

QVariantList Option::internalValueList() const
{
    if (d->option != nullptr) {
        return d->option->internalValueList();
    } else {
        return QVariantList();
    }
}

QVariant Option::value() const
{
    if (d->option != nullptr) {
        return d->option->value();
    } else {
        return QVariant();
    }
}

Option::OptionUnit Option::valueUnit() const
{
    if (d->option != nullptr) {
        return d->option->valueUnit();
    } else {
        return UnitNone;
    }
}

int Option::valueSize() const
{
    if (d->option != nullptr) {
        return d->option->valueSize();
    } else {
        return 0;
    }
}

bool Option::setValue(const QVariant &value)
{
    if (d->option != nullptr) {
        return d->option->setValue(value);
    } else {
        return false;
    }
}

bool Option::storeCurrentData()
{
    if (d->option != nullptr) {
        return d->option->storeCurrentData();
    } else {
        return false;
    }
}

bool Option::restoreSavedData()
{
    if (d->option != nullptr) {
        return d->option->restoreSavedData();
    } else {
        return false;
    }
}

} // namespace KSaneCore

#include "moc_option.cpp"
