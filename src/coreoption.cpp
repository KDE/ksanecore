/*
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "coreoption.h"
#include "coreoption_p.h"
#include "baseoption.h"

#include <ksanecore_debug.h>

namespace KSane
{

CoreOption::CoreOption(QObject *parent) : QObject(parent), d(std::unique_ptr<OptionPrivate>(new OptionPrivate()))
{
}

CoreOption::~CoreOption()
= default;

CoreOption::OptionState CoreOption::state() const
{
    if (d->option != nullptr) {
        return d->option->state();
    } else {
        return StateDisabled;
    }
}

QString CoreOption::name() const
{
    if (d->option != nullptr) {
        return d->option->name();
    } else {
        return QString();
    }
}

QString CoreOption::title() const
{
    if (d->option != nullptr) {
        return d->option->title();
    } else {
        return QString();
    }
}

QString CoreOption::description() const
{
    if (d->option != nullptr) {
        return d->option->description();
    } else {
        return QString();
    }
}

CoreOption::OptionType CoreOption::type() const
{  
    if (d->option != nullptr) {
        return d->option->type();
    } else {
        return TypeDetectFail;
    }
}

QVariant CoreOption::minimumValue() const
{  
    if (d->option != nullptr) {
        return d->option->minimumValue();
    } else {
        return QVariant();
    }
}

QVariant CoreOption::maximumValue() const
{
    if (d->option != nullptr) {
        return d->option->maximumValue();
    } else {
        return QVariant();
    }
}

QVariant CoreOption::stepValue() const
{
    if (d->option != nullptr) {
        return d->option->stepValue();
    } else {
        return QVariant();
    }
}

QVariantList CoreOption::valueList() const
{ 
    if (d->option != nullptr) {
        return d->option->valueList();
    } else {
        return QVariantList();
    }
}

QVariantList CoreOption::internalValueList() const
{
    if (d->option != nullptr) {
        return d->option->internalValueList();
    } else {
        return QVariantList();
    }
}

QVariant CoreOption::value() const
{
    if (d->option != nullptr) {
        return d->option->value();
    } else {
        return QVariant();
    }
}

CoreOption::OptionUnit CoreOption::valueUnit() const
{   
    if (d->option != nullptr) {
        return d->option->valueUnit();
    } else {
        return UnitNone;
    }
}

int CoreOption::valueSize() const
{
    if (d->option != nullptr) {
        return d->option->valueSize();
    } else {
        return 0;
    }
}

bool CoreOption::setValue(const QVariant &value)
{    
    if (d->option != nullptr) {
        return d->option->setValue(value);
    } else {
        return false;
    }
}
 
bool CoreOption::storeCurrentData()
{
    if (d->option != nullptr) {
        return d->option->storeCurrentData();
    } else {
        return false;
    }
}

bool CoreOption::restoreSavedData()
{
    if (d->option != nullptr) {
        return d->option->restoreSavedData();
    } else {
        return false;
    }
}

} // namespace KSane
