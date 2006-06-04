/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2005 Boudewijn <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * This implementation completely and utterly based on the gimp's bumpmap.c,
 * copyright:
 * Copyright (C) 1997 Federico Mena Quintero <federico@nuclecu.unam.mx>
 * Copyright (C) 1997-2000 Jens Lautenbacher <jtl@gimp.org>
 * Copyright (C) 2000 Sven Neumann <sven@gimp.org>
 *
 */

#include <stdlib.h>
#include <vector>

#include <QPoint>
#include <QLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QButtonGroup>
#include <QString>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>

#include <knuminput.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <kgenericfactory.h>

#include <kis_doc.h>
#include <kis_image.h>
#include <kis_iterators_pixel.h>
#include <kis_layer.h>
#include <kis_filter_registry.h>
#include <kis_global.h>
#include <kis_types.h>
#include <kis_layer.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_adjustment_layer.h>

#include <KoIntegerMaths.h>

#include "bumpmap.h"

#define MOD(x, y) \
  ((x) < 0 ? ((y) - 1 - ((y) - 1 - (x)) % (y)) : (x) % (y))

typedef KGenericFactory<KritaBumpmap> KritaBumpmapFactory;
K_EXPORT_COMPONENT_FACTORY( kritabumpmap, KritaBumpmapFactory( "krita" ) )

KritaBumpmap::KritaBumpmap(QObject *parent, const QStringList &)
        : KParts::Plugin(parent)
{
    setInstance(KritaBumpmapFactory::instance());


    if (parent->inherits("KisFilterRegistry")) {
        KisFilterRegistry * manager = dynamic_cast<KisFilterRegistry *>(parent);
        manager->add(KisFilterSP(new KisFilterBumpmap()));
    }
}

KritaBumpmap::~KritaBumpmap()
{
}

KisFilterBumpmap::KisFilterBumpmap() : KisFilter(id(), "map", i18n("&Bumpmap..."))
{
}

namespace {
    void convertRow(KisPaintDevice * orig, quint8 * row, qint32 x, qint32 y, qint32 w,  quint8 * lut, qint32 waterlevel)
    {
        KoColorSpace * csOrig = orig->colorSpace();

        KisHLineIteratorPixel origIt = orig->createHLineIterator(x, y, w, false);
        for (int i = 0; i < w; ++i) {
            row[0] = csOrig->intensity8(origIt.rawData());
            row[0] = lut[waterlevel + ((row[0] -  waterlevel) * csOrig->getAlpha(origIt.rawData())) / 255];

            ++row;
            ++origIt;
        }
    }

}

void KisFilterBumpmap::process(KisPaintDeviceSP src, KisPaintDeviceSP dst, KisFilterConfiguration* cfg, const QRect& rect)
{
    if (!src) return;
    if (!dst) return;
    if (!cfg) return;
    if (!rect.isValid()) return;
    if (rect.isNull()) return;
    if (rect.isEmpty()) return;

    KisBumpmapConfiguration * config = (KisBumpmapConfiguration*)cfg;

    qint32 lx, ly;       /* X and Y components of light vector */
    qint32 nz2, nzlz;    /* nz^2, nz*lz */
    qint32 background;   /* Shade for vertical normals */
    double  compensation; /* Background compensation */
    quint8 lut[256];     /* Look-up table for modes */

    double azimuth;
    double elevation;
    qint32 lz, nz;
    qint32 i;
    double n;

    // ------------------ Prepare parameters

    /* Convert to radians */
    azimuth   = M_PI * config->azimuth / 180.0;
    elevation = M_PI * config->elevation / 180.0;

    /* Calculate the light vector */
    lx = (qint32)(cos(azimuth) * cos(elevation) * 255.0);
    ly = (qint32)(sin(azimuth) * cos(elevation) * 255.0);

    lz = (qint32)(sin(elevation) * 255.0);

    /* Calculate constant Z component of surface normal */
    nz = (qint32)((6 * 255) / config->depth);
    nz2  = nz * nz;
    nzlz = nz * lz;

    /* Optimize for vertical normals */
    background = lz;

    /* Calculate darkness compensation factor */
    compensation = sin(elevation);

    /* Create look-up table for map type */
    for (i = 0; i < 256; i++)
    {
        switch (config->type)
        {
        case SPHERICAL:
            n = i / 255.0 - 1.0;
            lut[i] = (int) (255.0 * sqrt(1.0 - n * n) + 0.5);
            break;

        case SINUSOIDAL:
            n = i / 255.0;
            lut[i] = (int) (255.0 *
                    (sin((-M_PI / 2.0) + M_PI * n) + 1.0) /
                    2.0 + 0.5);
            break;

        case LINEAR:
            default:
            lut[i] = i;
        }

        if (config->invert)
            lut[i] = 255 - lut[i];
    }


    // Crate a grayscale layer from the bumpmap layer.
    QRect bmRect;
    KisPaintDevice * bumpmap;

    if (!config->bumpmap.isNull() && src->image()) {
        KisLayerSP l = src->image()->findLayer(config->bumpmap);
        KisPaintDeviceSP bumplayer = KisPaintDeviceSP(0);

        KisPaintLayer * pl = dynamic_cast<KisPaintLayer*>(l.data());
        if (pl) {
            bumplayer = pl->paintDevice();
        }
        else {
            KisGroupLayer * gl = dynamic_cast<KisGroupLayer*>(l.data());
            if (gl) {
                bumplayer = gl->projection(gl->extent());
            }
            else {
                KisAdjustmentLayer * al = dynamic_cast<KisAdjustmentLayer*>(l.data());
                if (al) {
                    bumplayer = al->cachedPaintDevice();
                }
            }
        }


        if (bumplayer) {
            bmRect = bumplayer->exactBounds();
            bumpmap = bumplayer.data();
        }
        else {
            bmRect = rect;
            bumpmap = src.data();
        }
     }
     else {
         bmRect = rect;
         bumpmap = src.data();
    }


    qint32 sel_h = rect.height();
    qint32 sel_w = rect.width();
    qint32 sel_x = rect.x();
    qint32 sel_y = rect.y();

    qint32 bm_h = bmRect.height();
    qint32 bm_w = bmRect.width();
    qint32 bm_x = bmRect.x();

    setProgressTotalSteps(sel_h);

    // ------------------- Map the bumps
    qint32 yofs1, yofs2, yofs3;

    // ------------------- Initialize offsets
    if (config->tiled) {
        yofs2 = MOD (config->yofs + sel_y, bm_h);
        yofs1 = MOD (yofs2 - 1, bm_h);
        yofs3 = MOD (yofs2 + 1,  bm_h);
    }
    else {
          yofs2 = CLAMP (config->yofs + sel_y, 0, bm_h - 1);
          yofs1 = yofs2;
          yofs3 = CLAMP (yofs2 + 1, 0, bm_h - 1);

    }

    // ---------------------- Load initial three bumpmap scanlines

    KoColorSpace * srcCs = src->colorSpace();
    Q3ValueVector<KoChannelInfo *> channels = srcCs->channels();

    // One byte per pixel, converted from the bumpmap layer.
    quint8 * bm_row1 = new quint8[bm_w];
    quint8 * bm_row2 = new quint8[bm_w];
    quint8 * bm_row3 = new quint8[bm_w];
    quint8 * tmp_row;

    convertRow(bumpmap, bm_row1, bm_x, yofs1, bm_w, lut, config->waterlevel);
    convertRow(bumpmap, bm_row2, bm_x, yofs2, bm_w, lut, config->waterlevel);
    convertRow(bumpmap, bm_row3, bm_x, yofs3, bm_w, lut, config->waterlevel);

    bool row_in_bumpmap;

    qint32 xofs1, xofs2, xofs3, shade, ndotl, nx, ny;
    for (int y = sel_y; y < sel_h + sel_y; y++) {

        row_in_bumpmap = (y >= - config->yofs && y < - config->yofs + bm_h);

        // Bumpmap

        KisHLineIteratorPixel dstIt = dst->createHLineIterator(rect.x(), y, sel_w, true);
        KisHLineIteratorPixel srcIt = src->createHLineIterator(rect.x(), y, sel_w, false);

        qint32 tmp = config->xofs + sel_x;
        xofs2 = MOD (tmp, bm_w);

        qint32 x = 0;
        //while (x < sel_w || cancelRequested()) {
        while (!srcIt.isDone() && !cancelRequested()) {
            if (srcIt.isSelected()) {
                // Calculate surface normal from bumpmap
                if (config->tiled || row_in_bumpmap &&
                    x >= - tmp&& x < - tmp + bm_w) {

                    if (config->tiled) {
                        xofs1 = MOD (xofs2 - 1, bm_w);
                        xofs3 = MOD (xofs2 + 1, bm_w);
                    }
                    else {
                        xofs1 = CLAMP (xofs2 - 1, 0, bm_w - 1);
                        xofs3 = CLAMP (xofs2 + 1, 0, bm_w - 1);
                    }

                    nx = (bm_row1[xofs1] + bm_row2[xofs1] + bm_row3[xofs1] -
                        bm_row1[xofs3] - bm_row2[xofs3] - bm_row3[xofs3]);
                    ny = (bm_row3[xofs1] + bm_row3[xofs2] + bm_row3[xofs3] -
                        bm_row1[xofs1] - bm_row1[xofs2] - bm_row1[xofs3]);


                }
                else {
                    nx = 0;
                    ny = 0;
                }

                // Shade

                if ((nx == 0) && (ny == 0)) {
                    shade = background;
                }
                else {
                    ndotl = (nx * lx) + (ny * ly) + nzlz;

                    if (ndotl < 0) {
                        shade = (qint32)(compensation * config->ambient);
                    }
                    else {
                        shade = (qint32)(ndotl / sqrt(nx * nx + ny * ny + nz2));
                        shade = (qint32)(shade + qMax(0, (int)((255 * compensation - shade)) * config->ambient / 255));
                    }
                }

                // Paint
                srcCs->darken(srcIt.rawData(), dstIt.rawData(), shade, config->compensate, compensation, 1);
            }
              if (++xofs2 == bm_w)
                xofs2 = 0;
            ++srcIt;
            ++dstIt;
            ++x;
        }


        // Next line
        if (config->tiled || row_in_bumpmap) {
            tmp_row = bm_row1;
            bm_row1 = bm_row2;
            bm_row2 = bm_row3;
            bm_row3 = tmp_row;

            if (++yofs2 == bm_h) {
                yofs2 = 0;
            }
            if (config->tiled) {
                yofs3 = MOD(yofs2 + 1, bm_h);
            }
            else {
                yofs3 = CLAMP(yofs2 + 1, 0, bm_h - 1);
            }

            convertRow(bumpmap, bm_row3, bm_x, yofs3, bm_w, lut, config->waterlevel);
        }

        incProgress();
    }

    delete [] bm_row1;
    delete [] bm_row2;
    delete [] bm_row3;
    setProgressDone();

}

KisFilterConfigWidget * KisFilterBumpmap::createConfigurationWidget(QWidget* parent, KisPaintDeviceSP dev)
{
    KisBumpmapConfigWidget * w = new KisBumpmapConfigWidget(this, dev, parent);


    return w;
}

KisFilterConfiguration * KisFilterBumpmap::configuration(QWidget * w)
{

    KisBumpmapConfigWidget * widget = dynamic_cast<KisBumpmapConfigWidget *>(w);
    if (widget == 0) {
        return new KisBumpmapConfiguration();
    }
    else {
        return widget->config();
    }
}

KisFilterConfiguration * KisFilterBumpmap::configuration()
{
    return new KisBumpmapConfiguration();
}


KisBumpmapConfiguration::KisBumpmapConfiguration()
    : KisFilterConfiguration( "bumpmap", 1 )
{
    bumpmap.clear();
    azimuth = 135.0;
    elevation = 45.0;
    depth = 3;
    xofs = 0;
    yofs = 0;
    waterlevel = 0;
    ambient = 0;
    compensate = true;
    invert = false;
    tiled = true;
    type = krita::LINEAR;
}
void KisBumpmapConfiguration::fromXML(const QString & s)
{
    KisFilterConfiguration::fromXML( s );

    bumpmap.clear();
    azimuth = 135.0;
    elevation = 45.0;
    depth = 3;
    xofs = 0;
    yofs = 0;
    waterlevel = 0;
    ambient = 0;
    compensate = true;
    invert = false;
    tiled = true;
    type = krita::LINEAR;

    QVariant v;

    v = getProperty("bumpmap");
    if (v.isValid()) { bumpmap = v.toString(); }
    v = getProperty("azimuth");
    if (v.isValid()) { azimuth = v.toDouble(); }
    v = getProperty("elevation");
    if (v.isValid()) { elevation = v.toDouble();}
    v = getProperty("depth");
    if (v.isValid()) { depth = v.toDouble(); }
    v = getProperty("xofs");
    if (v.isValid()) { xofs = v.toInt(); }
    v = getProperty("yofs");
    if (v.isValid()) { yofs = v.toInt();}
    v = getProperty("waterlevel");
    if (v.isValid()) { waterlevel = v.toInt();}
    v = getProperty("ambient");
    if (v.isValid()) { ambient = v.toInt();}
    v = getProperty("compensate");
    if (v.isValid()) { compensate = v.toBool(); }
    v = getProperty("invert");
    if (v.isValid()) { invert = v.toBool(); }
    v = getProperty("tiled");
    if (v.isValid()) { tiled = v.toBool();}
    v = getProperty("type");
    if (v.isValid()) { type = (enumBumpmapType)v.toInt(); }

}


QString KisBumpmapConfiguration::toString()
{
    m_properties.clear();

    //setProperty("bumpmap", QVariant(bumpmap));
    setProperty("azimuth", QVariant(azimuth));
    setProperty("elevation", QVariant(elevation));
    setProperty("depth", QVariant(depth));
    setProperty("xofs", QVariant(xofs));
    setProperty("yofs", QVariant(yofs));
    setProperty("waterlevel", QVariant(waterlevel));
    setProperty("ambient", QVariant(ambient));
    setProperty("compensate", QVariant(compensate));
    setProperty("invert", QVariant(invert));
    setProperty("tiled", QVariant(tiled));
    setProperty("type", QVariant(type));

    return KisFilterConfiguration::toString();
}

KisBumpmapConfigWidget::KisBumpmapConfigWidget(KisFilter * filter, KisPaintDeviceSP dev, QWidget * parent, const char * name, Qt::WFlags f)
    : KisFilterConfigWidget(parent, name, f),
      m_filter(filter),
      m_device(dev)
{
    Q_ASSERT(m_filter);
    Q_ASSERT(m_device);

    m_page = new WdgBumpmap(this);
    QHBoxLayout * l = new QHBoxLayout(this);
    Q_CHECK_PTR(l);

    l->addWidget(m_page);
    m_filter->setAutoUpdate(false);
    m_page->txtSourceLayer->setText( "" );
    connect( m_page->bnRefresh, SIGNAL(clicked()), SIGNAL(sigPleaseUpdatePreview()));
}

KisBumpmapConfiguration * KisBumpmapConfigWidget::config()
{
    KisBumpmapConfiguration * cfg = new KisBumpmapConfiguration();
    cfg->bumpmap = m_page->txtSourceLayer->text();
    cfg->azimuth = m_page->dblAzimuth->value();
    cfg->elevation = m_page->dblElevation->value();
    cfg->depth = m_page->dblDepth->value();
    cfg->xofs = m_page->intXOffset->value();
    cfg->yofs = m_page->intYOffset->value();
    cfg->waterlevel = m_page->intWaterLevel->value();
    cfg->ambient = m_page->intAmbient->value();
    cfg->compensate = m_page->chkCompensate->isChecked();
    cfg->invert = m_page->chkInvert->isChecked();
    cfg->tiled = m_page->chkTiled->isChecked();

    if (m_page->radioLinear->isChecked()) {
        cfg->type = LINEAR;
    } else if (m_page->radioSpherical->isChecked()) {
        cfg->type = SPHERICAL;
    } else {
        Q_ASSERT(m_page->radioSinusoidal->isChecked());
        cfg->type = SINUSOIDAL;
    }

    return cfg;
}

void KisBumpmapConfigWidget::setConfiguration(KisFilterConfiguration * config)
{
    KisBumpmapConfiguration * cfg = dynamic_cast<KisBumpmapConfiguration*>(config);
    if (!cfg) return;

    m_page->txtSourceLayer->setText( cfg->bumpmap );
    m_page->dblAzimuth->setValue(cfg->azimuth);
    m_page->dblElevation->setValue(cfg->elevation);
    m_page->dblDepth->setValue(cfg->depth);
    m_page->intXOffset->setValue(cfg->xofs);
    m_page->intYOffset->setValue(cfg->yofs);
    m_page->intWaterLevel->setValue(cfg->waterlevel);
    m_page->intAmbient->setValue(cfg->ambient);
    m_page->chkCompensate->setChecked(cfg->compensate);
    m_page->chkInvert->setChecked(cfg->invert);
    m_page->chkTiled->setChecked(cfg->tiled);

    switch (cfg->type) {
    default:
    case LINEAR:
        m_page->radioLinear->setChecked(true);
        break;
    case SPHERICAL:
        m_page->radioSpherical->setChecked(true);
        break;
    case SINUSOIDAL:
        m_page->radioSinusoidal->setChecked(true);
        break;
    }
}

#include "bumpmap.moc"
