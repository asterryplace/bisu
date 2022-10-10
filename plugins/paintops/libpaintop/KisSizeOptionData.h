/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISSIZEOPTIONDATA_H
#define KISSIZEOPTIONDATA_H

#include <KisCurveOptionData.h>
#include <kis_paintop_lod_limitations.h>

class PAINTOP_EXPORT KisSizeOptionData : public KisCurveOptionData
{
public:
    KisSizeOptionData(bool isCheckable = true, const QString &prefix = QString());
    KisPaintopLodLimitations lodLimitations() const;
};

#endif // KISSIZEOPTIONDATA_H
