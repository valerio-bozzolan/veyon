/*
 * italc_side_bar.h - header iTALC's side-bar
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _ITALC_SIDE_BAR_H
#define _ITALC_SIDE_BAR_H


#include <QtCore/QMap>
#include <QtGui/QBoxLayout>

#include "kmultitabbar.h"
#include "side_bar_widget.h"


class italcSideBar : public KMultiTabBar
{
	Q_OBJECT
public:
	italcSideBar( KMultiTabBarMode _bm, QWidget * _parent,
							QWidget * _splitter );

	virtual ~italcSideBar();

	inline int openedTab( void ) const
	{
		return( m_openedTab );
	}

	virtual void setTab( const int _id, const bool _state );

	int appendTab( sideBarWidget * _sbw, const int _id );

	inline QWidget * tabWidgetParent( void )
	{
		return( m_tabWidgetParent );
	}


private slots:
	void tabClicked( int _id );


private:
	QWidget * m_tabWidgetParent;
	QMap<int, QWidget *> m_widgets;
	int m_openedTab;
} ;


#endif
