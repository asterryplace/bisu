/*
 *  copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 *  this program is free software; you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation; either version 2 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program; if not, write to the free software
 *  foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
 */
#ifndef KIS_ROTATE_VISITOR_H_
#define KIS_ROTATE_VISITOR_H_

#include "kis_types.h"
#include "kis_progress_subject.h"

class QRect;
class KisPaintDevice;
class KisProgressDisplayInterface;

class KisRotateVisitor : public KisProgressSubject {
        typedef KisProgressSubject super;  
        
        /* Structs for the image rescaling routine */
	
public:
        KisRotateVisitor();
        ~KisRotateVisitor();

        void visitKisPaintDevice(KisPaintDevice* dev);

	
        void rotate(double angle, KisProgressDisplayInterface *m_progress);

        void shear(double angleX, double angleY, KisProgressDisplayInterface *m_progress);

        /// Returns true if completed, false if not completed for whatever reason
        bool rotateRight90(KisPaintDevice *src, KisPaintDevice *dst, QRect r, KisProgressDisplayInterface *m_progress);

        /// Returns true if completed, false if not completed for whatever reason
        bool rotateLeft90(KisPaintDevice *src, KisPaintDevice *dst, QRect r, KisProgressDisplayInterface *m_progress);

        /// Returns true if completed, false if not completed for whatever reason
        bool rotate180(KisPaintDevice *src, KisPaintDevice *dst, QRect r, KisProgressDisplayInterface *m_progress);

        
private:
        KisPaintDevice* m_dev;

	// Implement KisProgressSubject
	bool m_cancelRequested;
        virtual void cancel() { m_cancelRequested = true; }
        

        Q_INT32 xShearImage(double shearX, KisProgressDisplayInterface *m_progress, Q_INT32 progressTotal, Q_INT32 progresscurrent);
        Q_INT32 yShearImage(double shearY, KisProgressDisplayInterface *m_progress, Q_INT32 progressTotal, Q_INT32 progresscurrent);
        void xCropImage(double deltaX);
        void yCropImage(double deltaY);


        
};

inline KisRotateVisitor::KisRotateVisitor()
{
}

inline KisRotateVisitor::~KisRotateVisitor()
{
}

inline void KisRotateVisitor::visitKisPaintDevice(KisPaintDevice* dev)
{
        m_dev = dev;
}
#endif // KIS_ROTATE_VISITOR_H_
