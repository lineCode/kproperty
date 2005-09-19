/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004 Alexander Dymo <cloudtemple@mskat.net>
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "pixmapedit.h"
#include "editoritem.h"
#include "property.h"

#include <qlayout.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qcursor.h>
#include <qpushbutton.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qimage.h>
#include <qfiledialog.h>
#include <qtooltip.h>

#include <kdebug.h>
#include <kimageio.h>

#ifdef Q_WS_WIN
#include <win32_utils.h>
#include <krecentdirs.h>
#endif

#ifndef PURE_QT
#include <kfiledialog.h>
#include <klocale.h>
#include <kfiledialog.h>
#endif

using namespace KoProperty;

PixmapEdit::PixmapEdit(Property *property, QWidget *parent, const char *name)
 : Widget(property, parent, name)
{
	setHasBorders(false);

	m_edit = new QLabel(this, "m_edit");
	m_edit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_edit->setMinimumHeight(5);
	m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_edit->setBackgroundMode(Qt::PaletteBase);
	m_edit->setMouseTracking(true);
	setBackgroundMode(Qt::PaletteBase);

	m_button = new QPushButton(i18n("..."), this, "m_button");
	QToolTip::add(m_button, i18n("Insert image from file"));
	m_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	QFontMetrics fm(m_button->font());
	m_button->setFixedWidth(fm.width(m_button->text()+" "));
	m_button->setFocusPolicy(NoFocus);

	m_popup = new QLabel(0, "m_popup", Qt::WStyle_Customize|Qt::WStyle_NoBorder|Qt::WX11BypassWM|WStyle_StaysOnTop);
	m_popup->setPaletteBackgroundColor(m_popup->palette().active().base());
	m_popup->setFrameStyle(QFrame::Plain|QFrame::Box);
	m_popup->setMargin(2);
	m_popup->setLineWidth(1);
	m_popup->hide();

	setFocusWidget(m_edit);
	connect(m_button, SIGNAL(clicked()), this, SLOT(selectPixmap()));
}

PixmapEdit::~PixmapEdit()
{
	delete m_popup;
}

QVariant
PixmapEdit::value() const
{
	return m_pixmap;
}

void
PixmapEdit::setValue(const QVariant &value, bool emitChange)
{
	m_pixmap = value.toPixmap();
	if (m_pixmap.isNull() || (m_pixmap.height()<=height())) {
		m_edit->setPixmap(m_pixmap);
		m_previewPixmap = m_pixmap;
	}
	else {
		QImage img(m_pixmap.convertToImage());
		if (!QRect(QPoint(0,0), m_edit->size()*3).contains(m_pixmap.rect())) {
			img = img.smoothScale(m_edit->size()*3, QImage::ScaleMin);
			m_previewPixmap.convertFromImage(img);//preview pixmap is a bit larger
		}
		else {
			m_previewPixmap = m_pixmap;
		}
		img = img.smoothScale(m_edit->size(), QImage::ScaleMin);
		QPixmap pm;
		pm.convertFromImage(img);
		m_edit->setPixmap(pm);
	}
	if (emitChange)
		emit valueChanged(this);
}

void
PixmapEdit::drawViewer(QPainter *p, const QColorGroup &, const QRect &r, const QVariant &value)
{
	QRect r2(r);
	r2.setHeight(r2.height()+1);
	p->setClipRect(r2, QPainter::CoordPainter);
	p->setClipping(true);
	p->eraseRect(r2);
	if (value.toPixmap().isNull())
		return;
	if (m_recentlyPainted!=value) {
		kdDebug() << "___" << r << endl;
		m_recentlyPainted = value;
		m_scaledPixmap = value.toPixmap();
		QImage img(m_scaledPixmap.convertToImage());
		img = img.smoothScale(r.size()+QSize(0,2), QImage::ScaleMin);
		m_scaledPixmap.convertFromImage(img);
	}
	p->drawPixmap(r.topLeft().x(), //+KPROPEDITOR_ITEM_MARGIN,
		r.topLeft().y()+(r.height()-m_scaledPixmap.height())/2+1, m_scaledPixmap);
}

void
PixmapEdit::selectPixmap()
{
/*#ifdef PURE_QT
	QString url = QFileDialog::getOpenFileName();
	if (!url.isEmpty()) {
		m_edit->setPixmap(QPixmap(url));
		emit valueChanged(this);
	}
#endif*/
	QString caption( i18n("Insert Image From File (for \"%1\" property)").arg(property()->caption()) );
#ifdef Q_WS_WIN
	QString recentDir;
	QString fileName = QFileDialog::getOpenFileName(
		KFileDialog::getStartURL(":lastVisitedImagePath", recentDir).path(), 
		convertKFileDialogFilterToQFileDialogFilter(KImageIO::pattern(KImageIO::Reading)), 
		this, 0, caption);
#else
	KURL url( KFileDialog::getImageOpenURL(
		":lastVisitedImagePath", this, caption) );
	QString fileName = url.isLocalFile() ? url.path() : url.prettyURL();

	//! @todo download the file if remote, then set fileName properly
#endif

	if (fileName.isEmpty())
		return;
	QPixmap pm;
	if (!pm.load(fileName)) {
		//! @todo err msg
		return;
	}
	setValue(pm);

#ifdef Q_WS_WIN
	//save last visited path
	KURL url(fileName);
	if (url.isLocalFile())
		KRecentDirs::add(":lastVisitedImagePath", url.directory());
#endif
}

void
PixmapEdit::resizeEvent(QResizeEvent *e)
{
	Widget::resizeEvent(e);
	m_edit->move(0,0);
	m_edit->resize(e->size()-QSize(m_button->width(),-1));
	m_button->move(m_edit->width(),0);
	m_button->setFixedSize(m_button->width(), height());
}

bool
PixmapEdit::eventFilter(QObject *o, QEvent *ev)
{
	if(o == m_edit) {
		if(ev->type() == QEvent::MouseButtonPress)  {
			if(m_previewPixmap.height() <= m_edit->height()
				&& m_previewPixmap.width() <= m_edit->width())
				return false;

			m_popup->setPixmap(m_previewPixmap.isNull() ? m_pixmap : m_previewPixmap);
			m_popup->resize(m_previewPixmap.size()+QSize(2*3,2*3));
			m_popup->move(QCursor::pos()+QPoint(3,15));
			m_popup->show();
		}
		else if(ev->type() == QEvent::MouseButtonRelease || ev->type() == QEvent::Hide) {
			if(m_popup->isVisible())
				m_popup->hide();
		}
		else if(ev->type() == QEvent::KeyPress) {
			QKeyEvent* e = static_cast<QKeyEvent*>(ev);
			if((e->key() == Key_Enter) || (e->key()== Key_Space) || (e->key() == Key_Return)) {
				m_button->animateClick();
				return true;
			}
		}
	}

	return Widget::eventFilter(o, ev);
}

#include "pixmapedit.moc"
