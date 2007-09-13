/*
        Copyright (C) 2007 Kåre Särs <kare.sars@kolumbus.fi>

        This library is free software; you can redistribute it and/or
        modify it under the terms of the GNU Library General Public
        License as published by the Free Software Foundation; either
        version 2 of the License, or (at your option) any later version.

        This library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Library General Public License for more details.

        You should have received a copy of the GNU Library General Public License
        along with this library; see the file COPYING.LIB.  If not, write to
        the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
        Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <iostream>

#include "sane_option.h"

#define MIN_FOAT_STEP 0.001

//************************************************************
SaneOption::SaneOption(const SANE_Handle s_handle, const int opt_num):
        sane_handle(s_handle), opt_number(opt_num)
{
    lchebx = 0;
    lcombx = 0;
    frame = 0;
    lslider = 0;
    lfslider = 0;
    lentry = 0;
    sane_data = 0;
    type = SW_DETECT_FAIL;
    iVal=0;
    fVal=0;
    bVal=false;
    icon_color = 0;
    icon_gray = 0;
    icon_bw = 0;

    cstrl = new QStringList("ComboStringList");

    sane_option = sane_get_option_descriptor(sane_handle, opt_number);
    if (sane_option == 0) {
        printf("sane_option == 0!! ");
        return;
    }
    type = getWidgetType();

    // get the state for the widget
    sw_state = SW_STATE_SHOWN;
    if (((sane_option->cap & SANE_CAP_SOFT_DETECT) == 0) ||
          (sane_option->cap & SANE_CAP_INACTIVE) ||
          (sane_option->size == 0))
    {
        sw_state = SW_STATE_HIDDEN;
    }
    else if ((sane_option->cap & SANE_CAP_SOFT_SELECT) == 0) {
        sw_state = SW_STATE_DISABLED;
    }
    if (type == SW_GROUP) sw_state = SW_STATE_NO_CHANGE;

    readValue();
}

//************************************************************
void SaneOption::createWidget(QWidget *parent)
{
    float tmp_step;
    //printf("createWidget for opt(%d)\n", opt_number);
    if (sane_option == 0) {
        printf("createWidget:sane_option == 0!!\n");
        return;
    }

    if (frame != 0) delete(frame);

    switch(type)
    {
        case SW_GROUP:
            frame = new LabeledSeparator(parent, QString(sane_option->title));
            return;
        case SW_CHECKBOX:
            frame = lchebx = new LabeledCheckbox(parent, QString(sane_option->title));
            connect(lchebx, SIGNAL(toggled(bool)), this, SLOT(checkboxChanged(bool)));
            break;
        case SW_COMBO:
            cstrl = genComboStringList();
            frame = lcombx = new LabeledCombo(parent, QString(sane_option->title), *cstrl);
            connect(lcombx, SIGNAL(activated(int)), this, SLOT(comboboxChanged(int)));
            break;
        case SW_SLIDER:
            frame = lslider = new LabeledSlider(parent,
                    QString(sane_option->title),
                    sane_option->constraint.range->min,
                    sane_option->constraint.range->max,
                    sane_option->constraint.range->quant);
            lslider->setSuffix(unitString());
            connect(lslider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
            break;
        case SW_SLIDER_INT:
            frame = lslider = new LabeledSlider(parent,
                    QString(sane_option->title),
                    SW_INT_MIN,
                    SW_INT_MAX,
                    1);
            lslider->setSuffix(unitString());
            connect(lslider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
            break;
        case SW_F_SLIDER:
            tmp_step = SANE_UNFIX(sane_option->constraint.range->quant);
            if (tmp_step < MIN_FOAT_STEP) tmp_step = MIN_FOAT_STEP;

            frame = lfslider = new LabeledFSlider(parent,
                    QString(sane_option->title),
                    SANE_UNFIX(sane_option->constraint.range->min),
                    SANE_UNFIX(sane_option->constraint.range->max),
                    tmp_step);
            lfslider->setSuffix(unitString());
            min_change = lfslider->step()/2;
            connect(lfslider, SIGNAL(valueChanged(float)), this, SLOT(fsliderChanged(float)));
            break;
        case SW_F_SLIDER_FIX:

            frame = lfslider = new LabeledFSlider(parent,
                    QString(sane_option->title),
                    SW_FIXED_MIN,
                    SW_FIXED_MAX ,
                    MIN_FOAT_STEP);
            lfslider->setSuffix(unitString());
            min_change = lfslider->step()/2;
            connect(lfslider, SIGNAL(valueChanged(float)), this, SLOT(fsliderChanged(float)));
            break;
        case SW_ENTRY:
            frame = lentry = new LabeledEntry(parent, QString(sane_option->title));
            connect(lentry, SIGNAL(entryEdited(const QString&)),
                    this, SLOT(entryChanged(const QString&)));
            break;
        case SW_GAMMA:
            frame = lgamma = new LabeledGamma(parent, QString(sane_option->title),
                                              sane_option->size/sizeof(SANE_Word));
            connect(lgamma, SIGNAL(gammaTableChanged(const QVector<int> &)),
                    this, SLOT(gammaTableChanged(const QVector<int> &)));
            if (strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR_R) == 0) lgamma->setColor(Qt::red);
            if (strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR_G) == 0) lgamma->setColor(Qt::green);
            if (strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR_B) == 0) lgamma->setColor(Qt::blue);
            break;
        case SW_DETECT_FAIL:
            frame = new LabeledSeparator(parent, ">>> " +
                    QString().sprintf("%d \"", opt_number) +
                            QString(sane_option->title)+"\" <<<");
            printf("SW_DETECT_FAIL opt(%d), %s\n", opt_number, sane_option->title);
            break;
    }

    if (sw_state == SW_STATE_HIDDEN) frame->hide();
    else {
        frame->show();
        frame->setEnabled(sw_state == SW_STATE_SHOWN);
    }

}
//************************************************************
SaneOptWidget_t SaneOption::getWidgetType(void)
{
    switch (sane_option->constraint_type)
    {
        case SANE_CONSTRAINT_NONE:
            switch(sane_option->type)
            {
                case SANE_TYPE_BOOL:
                    return SW_CHECKBOX;
                case SANE_TYPE_INT:
                    if (sane_option->size == sizeof(SANE_Word)) return SW_SLIDER_INT;
                case SANE_TYPE_FIXED:
                    if (sane_option->size == sizeof(SANE_Word)) return SW_F_SLIDER_FIX;
                case SANE_TYPE_BUTTON:
                    return SW_DETECT_FAIL;
                case SANE_TYPE_STRING:
                    return SW_ENTRY;
                case SANE_TYPE_GROUP:
                    return SW_GROUP;
            }
            break;
        case SANE_CONSTRAINT_RANGE:
            switch(sane_option->type)
            {
                case SANE_TYPE_BOOL:
                    return SW_CHECKBOX;
                case SANE_TYPE_INT:
                    if (sane_option->size == sizeof(SANE_Word)) return SW_SLIDER;
                    //printf("array of %d elements min=%d max=%d\n",
                    //       sane_option->size/sizeof(SANE_Word),
                    //               sane_option->constraint.range->min,
                    //               sane_option->constraint.range->max);
                    if ((strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR) == 0) ||
                         (strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR_R) == 0) ||
                         (strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR_G) == 0) ||
                         (strcmp(sane_option->name, SANE_NAME_GAMMA_VECTOR_B) == 0))
                    {
                        return SW_GAMMA;
                    }
                    break;
                case SANE_TYPE_FIXED:
                    if (sane_option->size == sizeof(SANE_Word)) return SW_F_SLIDER;
                    break;
                case SANE_TYPE_STRING:
                case SANE_TYPE_BUTTON:
                    return SW_DETECT_FAIL;
                case SANE_TYPE_GROUP:
                    return SW_GROUP;
            }
            break;
        case SANE_CONSTRAINT_WORD_LIST:
        case SANE_CONSTRAINT_STRING_LIST:
            return SW_COMBO;
    }
    return SW_DETECT_FAIL;
}

//************************************************************
QString SaneOption::name(void)
{
    if (sane_option == 0) return QString("");
    return QString(sane_option->name);
}

//************************************************************
QString SaneOption::unitString(void)
{
    switch(sane_option->unit)
    {
        case SANE_UNIT_NONE:    return QString("");
        case SANE_UNIT_PIXEL:   return QString(" Pixel");
        case SANE_UNIT_BIT:     return QString(" Bit");
        case SANE_UNIT_MM:      return QString(" mm");
        case SANE_UNIT_DPI:     return QString(" DPI");
        case SANE_UNIT_PERCENT: return QString(" %");
        case SANE_UNIT_MICROSECOND: return QString(" usec");
    }
    return QString("");
}


//************************************************************
SaneOption::~SaneOption(void)
{
}


//************************************************************
QStringList *SaneOption::genComboStringList(void)
{
    int i;

    cstrl->clear();

    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
            for (i=1; i<=sane_option->constraint.word_list[0]; i++)
            {
                *cstrl += (QString().sprintf("%d", sane_option->constraint.word_list[i]) +
                        unitString());
            }
            break;
        case SANE_TYPE_FIXED:
            for (i=1; i<=sane_option->constraint.word_list[0]; i++)
            {
                *cstrl += (QString().sprintf("%f", SANE_UNFIX(sane_option->constraint.word_list[i])) +
                        unitString());
            }
            break;
        case SANE_TYPE_STRING:
            i=0;
            while (sane_option->constraint.string_list[i] != 0)
            {
                *cstrl += getSaneComboString((unsigned char *)sane_option->constraint.string_list[i]);
                i++;
            }
            break;
        default :
            *cstrl += "NOT HANDELED";
    }
    return cstrl;
}

//************************************************************
QString SaneOption::getSaneComboString(unsigned char *data)
{
    QString tmp;

    if (type != SW_COMBO) {
        //printf("getSaneComboString: type != SW_COMBO\n");
        return QString("");
    }

    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
            return QString().sprintf("%d", (int)toSANE_Word(data)) + unitString();
        case SANE_TYPE_FIXED:
            return QString().sprintf("%f", SANE_UNFIX(toSANE_Word(data))) + unitString();
        case SANE_TYPE_STRING:
            tmp = QString((char*)data);
            // FIXME clean the end of the string !!
            if (tmp.length() > 25) {
                tmp = tmp.left(22);
                tmp += "...";
            }
            tmp += unitString();
            return tmp;
        default :
            break;
    }
    return QString("");
}

//************************************************************
bool SaneOption::writeData(unsigned char *data)
{
    SANE_Status status;
    SANE_Int res;

/*    printf("write data=0x");
    for (int i=0; i<sane_option->size; i++) {
        if (i%4 == 0) printf(" ");
        if (i%64 == 0) printf("\n");
        printf("%02x", data[i]);
    }*/
    status = sane_control_option (sane_handle, opt_number, SANE_ACTION_SET_VALUE, data, &res);
    //printf("'%20.20s'(%2d) status=%d, res=%d", sane_option->name, opt_number, status, res);
    //printf("data=0x%02x%02x%02x%02x\n", data[0], data[1], data[2], data[3]);
    if (status != SANE_STATUS_GOOD) {
        printf("writeData: '%s' sane_control_option returned %d\n", sane_option->name, status);
        return false;
    }
    if ((res & SANE_INFO_INEXACT) && (frame != 0)) {
        //printf("writeData: write was inexact. Reload value just in case...\n");
        readValue();
    }

    if (res & SANE_INFO_RELOAD_OPTIONS) {
        emit optsNeedReload();
        // optReload reloads also the values
    }
    else if (res & SANE_INFO_RELOAD_PARAMS) {
        // 'else if' because with optReload we force also valReload :)
        emit valsNeedReload();
    }

    return true;
}

//************************************************************
void SaneOption::checkboxChanged(bool toggled)
{
    unsigned char data[4];

    fromSANE_Word(data, (toggled) ? 1:0);
    writeData(data);

}

//************************************************************
void SaneOption::comboboxChanged(int i)
{
    unsigned char data[sane_option->size];

    //printf("comboboxChanged: ");
    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
        case SANE_TYPE_FIXED:
            fromSANE_Word(data, sane_option->constraint.word_list[i+1]);
            break;
        case SANE_TYPE_STRING:
            strncpy((char*)data, sane_option->constraint.string_list[i], sane_option->size);
            break;
        default:
            printf("comboboxChanged(index): can not handle type(%d)\n", sane_option->type);
            break;
    }
    writeData(data);
}

//************************************************************
bool SaneOption::comboboxChanged(float value)
{
    unsigned char data[sane_option->size];
    SANE_Word fixed;

    //printf("comboboxChanged: ");
    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
            fromSANE_Word(data, (int)value);
            break;
        case SANE_TYPE_FIXED:
            fixed = SANE_FIX(value);
            fromSANE_Word(data, fixed);
            break;
        default:
            printf("comboboxChanged(float): can only handle SANE_TYPE_INT and SANE_TYPE_FIXED\n");
            return false;
    }
    writeData(data);
    return true;
}

//************************************************************
bool SaneOption::comboboxChanged(const QString &value)
{
    unsigned char data[sane_option->size];
    SANE_Word fixed;
    int i;
    float f;
    bool ok;
    QString tmp;

    //printf("comboboxChanged: ");
    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
            i = value.toInt(&ok);
            if (ok == false) return false;
            fromSANE_Word(data, i);
            break;
        case SANE_TYPE_FIXED:
            f = value.toFloat(&ok);
            if (ok == false) return false;
            fixed = SANE_FIX(f);
            fromSANE_Word(data, fixed);
            break;
        case SANE_TYPE_STRING:
            i = 0;
            while (sane_option->constraint.string_list[i] != 0) {
                tmp = getSaneComboString((unsigned char *)sane_option->constraint.string_list[i]);
                if (value == tmp) {
                    strncpy((char*)data, sane_option->constraint.string_list[i], sane_option->size);
                    //std::cout << "->>" << qPrintable(tmp) << std::endl;
                    break;
                }
                i++;
            }
            if (sane_option->constraint.string_list[i] == 0) return false;
            break;
        default:
            printf("comboboxChanged: can only handle SANE_TYPE_INT and SANE_TYPE_FIXED\n");
            return false;
    }
    writeData(data);
    return true;
}

//************************************************************
void SaneOption::sliderChanged(int val)
{
    unsigned char data[4];

    if (val == iVal) return;
    iVal = val;
    fromSANE_Word(data, val);
    writeData(data);
}

//************************************************************
void SaneOption::fsliderChanged(float val)
{
    unsigned char data[4];
    SANE_Word fixed;

    if (((val-fVal) >= min_change) || ((fVal-val) >= min_change)) {
        //printf("opt(%s): fsliderChanged(%f - %f)\n", sane_option->name, fVal, val);
        fVal = val;
        fixed = SANE_FIX(val);
        fromSANE_Word(data, fixed);
        writeData(data);
    }
}

//************************************************************
void SaneOption::entryChanged(const QString& text)
{
    char data[sane_option->size];

    QString tmp("");
    tmp += text.left(sane_option->size);
    if (tmp != text) lentry->setText(tmp);
    strcpy(data, tmp.toLatin1());
    writeData((unsigned char *)data);

}

//************************************************************
void SaneOption::gammaTableChanged(const QVector<int> &gam_tbl)
{
    //printf("Gamma table (%s) changed\n", sane_option->name);
    writeData((unsigned char *)gam_tbl.data());
}

//************************************************************
void SaneOption::readOption(void)
{
    float tmp_step;
    int tmp_size;

    sane_option = sane_get_option_descriptor(sane_handle, opt_number);

    //printf("readOption(%2d):'%20.20s' ", opt_number, sane_option->title);
    //printf((sane_option->cap & SANE_CAP_SOFT_SELECT)     ? "| S_SEL ":"|       ");
    //printf((sane_option->cap & SANE_CAP_HARD_SELECT)     ? "| H_SEL ":"|       ");
    //printf((sane_option->cap & SANE_CAP_SOFT_DETECT)     ? "| S_DET ":"|       ");
    //printf((sane_option->cap & SANE_CAP_EMULATED)        ? "| EMULA ":"|       ");
    //printf((sane_option->cap & SANE_CAP_AUTOMATIC)       ? "| AUTOM ":"|       ");
    //printf((sane_option->cap & SANE_CAP_INACTIVE)        ? "| INACT ":"|       ");
    //printf((sane_option->cap & SANE_CAP_ADVANCED)        ? "| ADVAN ":"|       ");
    //printf((sane_option->cap & SANE_CAP_ALWAYS_SETTABLE) ? "| ALW_S |":"|      |");

    // get the state for the widget
    sw_state = SW_STATE_SHOWN;
    if (((sane_option->cap & SANE_CAP_SOFT_DETECT) == 0) ||
          (sane_option->cap & SANE_CAP_INACTIVE) ||
          (sane_option->size == 0))
    {
        //printf("Hidden! ");
        sw_state = SW_STATE_HIDDEN;
    }
    else if ((sane_option->cap & SANE_CAP_SOFT_SELECT) == 0) {
        //printf("Disabled!");
        sw_state = SW_STATE_DISABLED;
    }
    if (type == SW_GROUP) {
        //printf("Group!");
        sw_state = SW_STATE_NO_CHANGE;
    }
    //printf("\n");

    if (frame == 0) return;

    switch(type)
    {
        case SW_COMBO:
            cstrl = genComboStringList();
            lcombx->clear();
            lcombx->addItems(*cstrl);
            if (icon_color != 0) {
                lcombx->setIcon(*icon_color,
                                getSaneComboString((unsigned char*)SANE_VALUE_SCAN_MODE_COLOR));
            }
            if (icon_gray != 0) {
                lcombx->setIcon(*icon_gray,
                                getSaneComboString((unsigned char*)SANE_VALUE_SCAN_MODE_GRAY));
            }
            if (icon_bw != 0) {
                lcombx->setIcon(*icon_bw,
                                getSaneComboString((unsigned char*)SANE_VALUE_SCAN_MODE_LINEART));
            }
            break;
        case SW_SLIDER:
            lslider->setRange(sane_option->constraint.range->min,
                              sane_option->constraint.range->max);
            lslider->setStep(sane_option->constraint.range->quant);
            lslider->setSuffix(unitString());
            break;
        case SW_SLIDER_INT:
            lslider->setRange(SW_INT_MIN, SW_INT_MAX);
            lslider->setStep(1);
            lslider->setSuffix(unitString());
            break;
        case SW_F_SLIDER:
            lfslider->setRange(SANE_UNFIX(sane_option->constraint.range->min),
                               SANE_UNFIX(sane_option->constraint.range->max));

            tmp_step = SANE_UNFIX(sane_option->constraint.range->quant);
            if (tmp_step < MIN_FOAT_STEP) tmp_step = MIN_FOAT_STEP;
            lfslider->setStep(tmp_step);
            min_change = lfslider->step()/2;

            lfslider->setSuffix(unitString());
            break;
        case SW_F_SLIDER_FIX:
            lfslider->setRange(SW_FIXED_MIN, SW_FIXED_MAX);
            lfslider->setStep(MIN_FOAT_STEP);
            min_change = lfslider->step()/2;
            lfslider->setSuffix(unitString());
            break;
        case SW_GAMMA:
            tmp_size = sane_option->size/sizeof(SANE_Word);
            if (lgamma->gammaTablePtr().size() != tmp_size) {
                lgamma->setSize(tmp_size);
            }
            break;
        case SW_GROUP:
        case SW_CHECKBOX:
        case SW_ENTRY:
        case SW_DETECT_FAIL:
            // no changes
            break;
    }

    if (sw_state == SW_STATE_HIDDEN) frame->hide();
    else if (sw_state != SW_STATE_NO_CHANGE) {
        frame->show();
        frame->setEnabled(sw_state == SW_STATE_SHOWN);
    }

}

//************************************************************
void SaneOption::readValue(void)
{
    // check if we can read the value
    if (type == SW_GROUP) return;
    if (sw_state == SW_STATE_HIDDEN) return;

    // read that current value
    unsigned char data[sane_option->size];
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option (sane_handle, opt_number, SANE_ACTION_GET_VALUE, data, &res);

    //printf("opt(%2d):'%15.15s', ", opt_number, sane_option->name);
    //printf("st=%d, res=%d ", status, res);

    if (status != SANE_STATUS_GOOD) {
        printf("\n");
        return;
    }

    switch(type)
    {
        case SW_GROUP:
            break;
        case SW_CHECKBOX:
            //printf("checked = %d\n", (int)toSANE_Word(data));
            bVal = (toSANE_Word(data) != 0) ? true:false;
            if (lchebx != 0) {
                lchebx->setChecked(bVal);
            }
            emit cbValueRead(bVal);
            break;
        case SW_COMBO:
            //printf("Combo = '%s'\n", qPrintable(getSaneComboString(data)));
            if (lcombx != 0) {
                lcombx->setCurrentText(getSaneComboString(data));
            }
            break;
        case SW_SLIDER:
        case SW_SLIDER_INT:
            iVal = toSANE_Word(data);
            //printf("Slider V = %d\n",(int)iVal);
            if ((lslider != 0) &&  (lslider->value() != (int)iVal)) {
                lslider->setValue((int)iVal);
            }
            emit iValueRead((int)iVal);
            break;
        case SW_F_SLIDER:
        case SW_F_SLIDER_FIX:
            fVal = SANE_UNFIX(toSANE_Word(data));
            //printf("Slider F V = %f\n", fVal);
            if (lfslider != 0) {
                if (((lfslider->value() - fVal) >= min_change) ||
                      ((fVal- lfslider->value()) >= min_change) )
                {
                    lfslider->setValue(fVal);
                }
            }
            emit fValueRead(fVal);
            break;
        case SW_ENTRY:
            //printf("Text Entry '%s'\n", qPrintable(QString((char*)data)));
            if (lentry != 0) {
                lentry->setText(QString((char*)data));
            }
            break;
        case SW_GAMMA:
            // Unfortunately gamma table to brigthness, contrast and gamma is
            // not easy nor fast.. ergo not done
            //printf("Gamma table (%s):", sane_option->name);
            //for (int i=0; i<sane_option->size; i+=4) {
            //    if (i%64 == 0) printf("\n");
            //    printf("%d ", toSANE_Word(&data[i]));
            //}
            //printf("\n");
            break;
        case SW_DETECT_FAIL:
            //printf("Unhandeled\n");
            break;
    }

}

//************************************************************
SANE_Word SaneOption::toSANE_Word(unsigned char *data)
{
    SANE_Word tmp;
    tmp = data[0];
    tmp += data[1]<<8;
    tmp += data[2]<<16;
    tmp += data[3]<<24;

    return tmp;
}

//************************************************************
void SaneOption::fromSANE_Word(unsigned char *data, SANE_Word from)
{
    data[0] = (from & 0x000000FF);
    //printf("data[0]
    data[1] = (from & 0x0000FF00)>>8;
    data[2] = (from & 0x00FF0000)>>16;
    data[3] = (from & 0xFF000000)>>24;
}

//************************************************************
bool SaneOption::getMaxValue(float *max)
{
    int last;
    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
            switch (sane_option->constraint_type)
            {
                case SANE_CONSTRAINT_RANGE:
                    *max = (float)(sane_option->constraint.range->max);
                    return true;
                case SANE_CONSTRAINT_WORD_LIST:
                    last = sane_option->constraint.word_list[0];
                    *max = (float)(sane_option->constraint.word_list[last]);
                    return true;
                case SANE_CONSTRAINT_NONE:
                    if (sane_option->size == sizeof(SANE_Word)) {
                        // FIXME precision is lost.
                        *max = (float)SW_INT_MAX;
                        return true;
                    }
                default:
                    printf("Constraint must be range or word list!\n");
            }
            break;
        case SANE_TYPE_FIXED:
            switch (sane_option->constraint_type)
            {
                case SANE_CONSTRAINT_RANGE:
                    *max = SANE_UNFIX(sane_option->constraint.range->max);
                    return true;
                case SANE_CONSTRAINT_WORD_LIST:
                    last = sane_option->constraint.word_list[0];
                    *max = SANE_UNFIX(sane_option->constraint.word_list[last]);
                    return true;
                case SANE_CONSTRAINT_NONE:
                    if (sane_option->size == sizeof(SANE_Word)) {
                        *max = SW_FIXED_MAX;
                        return true;
                    }
                default:
                    printf("Constraint must be range or word list!\n");
            }
            break;
        default:
            printf("Bad type %d for unit mm!!\n", sane_option->type);
    }
    return false;
}

//************************************************************
bool SaneOption::getValue(float *val)
{
    // check if we can read the value
    if (type == SW_GROUP) return false;
    if (sw_state == SW_STATE_HIDDEN) return false;

    // read that current value
    unsigned char data[sane_option->size];
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option (sane_handle, opt_number, SANE_ACTION_GET_VALUE, data, &res);
    if (status != SANE_STATUS_GOOD) {
        printf("getValue(float): '%s' sane_control_option returned %d\n", sane_option->name, status);
        return false;
    }
    //printf("getValue(float)%2d: '%20.20s' status = %d, res=%d\n", opt_number, sane_option->name, status, res);

    switch (sane_option->type)
    {
        case SANE_TYPE_INT:
            *val = (float)toSANE_Word(data);
            return true;
        case SANE_TYPE_FIXED:
            *val = SANE_UNFIX(toSANE_Word(data));
            return true;
        default:
            printf("getValue(float): Type %d not supported!\n", sane_option->type);
    }
    return false;
}

//************************************************************
bool SaneOption::setValue(float value)
{
    //printf("setValue(float): '%s' value: %f\n", sane_option->name, value);
    switch (type)
    {
        case SW_SLIDER:
            sliderChanged((int)(value+0.5));
            if (lslider != 0) {
                lslider->setValue((int)value);
            }
            return true;
        case SW_F_SLIDER:
            fsliderChanged(value);
            if (lfslider != 0) {
                lfslider->setValue(value);
            }
            return true;
        case SW_COMBO:
            if (comboboxChanged(value) == false){
                return false;
            }
            if (lcombx != 0) {
                // force gui update
                readValue();
            }
            return true;
        default:
            std::cout << "Only options of type slider and fslider are supported" << std::endl;
    }
    return false;
}

//************************************************************
bool SaneOption::setChecked(bool check)
{
    //std::cout << "-> SetChecked = " << check << std::endl;
    switch (type)
    {
        case SW_CHECKBOX:
            checkboxChanged(check);
            if (lchebx != 0) {
                readValue();
            }
            return true;
        default:
            std::cout << "Only works on boolean options" << std::endl;
    }
    return false;
}

//************************************************************
bool SaneOption::storeCurrentData(void)
{
    SANE_Status status;
    SANE_Int res;

    // check if we can read the value
    if (type == SW_GROUP) return false;
    if (sw_state == SW_STATE_HIDDEN) return false;

    // read that current value
    if (sane_data != 0) delete (sane_data);
    sane_data = (unsigned char *)malloc(sane_option->size);
    status = sane_control_option (sane_handle, opt_number, SANE_ACTION_GET_VALUE, sane_data, &res);
    if (status != SANE_STATUS_GOOD) {
        printf("storeCurrentData: '%s' sane_control_option returned %d\n", sane_option->name, status);
        return false;
    }
    return true;
}


//************************************************************
bool SaneOption::restoreSavedData(void)
{
    // check if we have saved any data
    if (sane_data == 0) return false;

    // check if we can write the value
    if (type == SW_GROUP) return false;
    if (sw_state == SW_STATE_HIDDEN) return false;
    if (sw_state == SW_STATE_DISABLED) return false;

    writeData(sane_data);

    return true;

}

//************************************************************
bool SaneOption::getValue(QString *val)
{
    // check if we can read the value
    if (type == SW_GROUP) return false;
    if (sw_state == SW_STATE_HIDDEN) return false;

    // read that current value
    unsigned char data[sane_option->size];
    SANE_Status status;
    SANE_Int res;
    status = sane_control_option (sane_handle, opt_number, SANE_ACTION_GET_VALUE, data, &res);
    if (status != SANE_STATUS_GOOD) {
        printf("getValue(QString): '%s' sane_control_option returned %d\n",
               sane_option->name, status);
        return false;
    }
    //printf("getValue(QString): opt_number=%2d status = %d, res=%d ", opt_number, status, res);

    switch(type)
    {
        case SW_CHECKBOX:
            *val = (toSANE_Word(data) != 0) ? QString("true") :QString("false");
            break;
        case SW_COMBO:
            *val = getSaneComboString(data);
            break;
        case SW_SLIDER:
            *val = QString().sprintf("%d", (int)toSANE_Word(data));
            break;
        case SW_F_SLIDER:
            *val = QString().sprintf("%f", SANE_UNFIX( toSANE_Word(data)));
            break;
        case SW_ENTRY:
            *val = QString((char*)data);
            break;
        default:
            printf("getValue(QString):'%s' type(%d) is not supported\n", sane_option->name, type);
            return false;
    }
    return true;
}

//************************************************************
bool SaneOption::setValue(const QString &val)
{
    QString tmp;
    bool ok;
    int i;
    float f;

    //printf("setValue(QString): '%s' ->'%s'\n", sane_option->name, qPrintable(val));

    // check if we can vrite to the value
    if (type == SW_GROUP) return false;
    if (sw_state == SW_STATE_HIDDEN) return false;
    if (sw_state == SW_STATE_DISABLED) return false;

    if (getValue(&tmp) == false) return false;

    if (tmp == val) return true; // no update needed

    switch(type)
    {
        case SW_CHECKBOX:
            checkboxChanged(val == QString("true"));
            if (lchebx != 0) {
                readValue();
            }
            break;
        case SW_COMBO:
            comboboxChanged(val);
            if (lcombx != 0) {
                readValue();
            }
            break;
        case SW_SLIDER:
            i = val.toInt(&ok);
            if (ok == false) return false;
            sliderChanged(i);
            if (lslider != 0) {
                readValue();
            }
            break;
        case SW_F_SLIDER:
            f = val.toFloat(&ok);
            if (ok == false) return false;
            fsliderChanged(f);
            if (lfslider != 0) {
                readValue();
            }
            break;
        case SW_ENTRY:
            entryChanged(val);
            if (lentry != 0) {
                readValue();
            }
            break;
        default:
            printf("setValue(QString): type(%d) is not supported\n", type);
            return false;
    }
    return true;
}


//************************************************************
bool SaneOption::setIconColorMode(const QIcon &icon)
{
    if (icon_color != 0) {
        delete icon_color;
    }
    icon_color = new QIcon(icon);
    if ((strcmp(sane_option->name, SANE_NAME_SCAN_MODE) != 0) ||
       (lcombx == 0))
    {
        return false;
    }
    return lcombx->setIcon(icon, getSaneComboString((unsigned char*)SANE_VALUE_SCAN_MODE_COLOR));
}

//************************************************************
bool SaneOption::setIconGrayMode(const QIcon &icon)
{
    if (icon_gray != 0) {
        delete icon_gray;
    }
    icon_gray = new QIcon(icon);
    if ((strcmp(sane_option->name, SANE_NAME_SCAN_MODE) != 0) ||
         (lcombx == 0))
    {
        return false;
    }
    return lcombx->setIcon(icon, getSaneComboString((unsigned char*)SANE_VALUE_SCAN_MODE_GRAY));
}

//************************************************************
bool SaneOption::setIconBWMode(const QIcon &icon)
{
    if (icon_bw != 0) {
        delete icon_bw;
    }
    icon_bw = new QIcon(icon);
    if ((strcmp(sane_option->name, SANE_NAME_SCAN_MODE) != 0) ||
         (lcombx == 0))
    {
        return false;
    }
    return lcombx->setIcon(icon, getSaneComboString((unsigned char*)SANE_VALUE_SCAN_MODE_LINEART));
}

