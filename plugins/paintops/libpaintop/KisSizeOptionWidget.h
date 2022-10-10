/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISSIZEOPTIONWIDGET_H
#define KISSIZEOPTIONWIDGET_H

#include <KisCurveOptionWidget2.h>
#include <KisSizeOptionData.h>

class PAINTOP_EXPORT KisSizeOptionWidget : public KisCurveOptionWidget2
{
    Q_OBJECT
public:
    using data_type = KisSizeOptionData;

    KisSizeOptionWidget(lager::cursor<KisSizeOptionData> optionData);
    KisSizeOptionWidget(lager::cursor<KisSizeOptionData> optionData, KisPaintOpOption::PaintopCategory categoryOverride);
    ~KisSizeOptionWidget();

    lager::reader<KisPaintopLodLimitations> lodLimitationsReader() const;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};


#endif // KISSIZEOPTIONWIDGET_H
