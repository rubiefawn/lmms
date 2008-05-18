#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_chain.cpp - class for processing and effects chain
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "effect_chain.h"
#include "effect.h"
#include "engine.h"
#include "automatable_model_templates.h"
#include "track.h"
#include "debug.h"




effectChain::effectChain( track * _track ) :
	model( _track ),
	m_track( _track ),
	m_enabledModel( FALSE )
{
}




effectChain::~effectChain()
{
	clear();
}




void effectChain::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "enabled", m_enabledModel.value() );
	_this.setAttribute( "numofeffects", m_effects.count() );
	for( effectList::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++ )
	{
		QDomElement ef = ( *it )->saveState( _doc, _this );
		ef.setAttribute( "name", ( *it )->getDescriptor()->name );
		ef.setAttribute( "key", ( *it )->getKey().dumpBase64() );
	}
}




void effectChain::loadSettings( const QDomElement & _this )
{
	clear();

	m_enabledModel.setValue( _this.attribute( "enabled" ).toInt() );

	const int plugin_cnt = _this.attribute( "numofeffects" ).toInt();

	QDomNode node = _this.firstChild();
	int fx_loaded = 0;
	while( !node.isNull() && fx_loaded < plugin_cnt )
	{
		if( node.isElement() && node.nodeName() == "effect" )
		{
			QDomElement cn = node.toElement();
			const QString name = cn.attribute( "name" );
			// we have this really convenient key-ctor
			// which takes a QString and decodes the
			// base64-data inside :-)
			effectKey key( cn.attribute( "key" ) );
			effect * e = effect::instantiate( name, this, &key );
			m_effects.push_back( e );
			// TODO: somehow detect if effect is sub-plugin-capable
			// but couldn't load sub-plugin with requested key
			if( node.isElement() )
			{
				if( e->nodeName() == node.nodeName() )
				{
					e->restoreState( node.toElement() );
				}
			}
			++fx_loaded;
		}
		node = node.nextSibling();
	}

	emit dataChanged();
}





void effectChain::appendEffect( effect * _effect )
{
	engine::getMixer()->lock();
	_effect->m_enabledModel.setTrack( m_track );
	_effect->m_wetDryModel.setTrack( m_track );
	_effect->m_gateModel.setTrack( m_track );
	_effect->m_autoQuitModel.setTrack( m_track );
	m_effects.append( _effect );
	engine::getMixer()->unlock();
	emit dataChanged();
}



void effectChain::removeEffect( effect * _effect )
{
	engine::getMixer()->lock();
	m_effects.erase( qFind( m_effects.begin(), m_effects.end(), _effect ) );
	engine::getMixer()->unlock();
}




void effectChain::moveDown( effect * _effect )
{
	if( _effect != m_effects.last() )
	{
		int i = 0;
		for( effectList::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++, i++ )
		{
			if( *it == _effect )
			{
				break;
			}
		}
		
		effect * temp = m_effects[i + 1];
		m_effects[i + 1] = _effect;
		m_effects[i] = temp;	
	}
}




void effectChain::moveUp( effect * _effect )
{
	if( _effect != m_effects.first() )
	{
		int i = 0;
		for( effectList::iterator it = m_effects.begin(); 
					it != m_effects.end(); it++, i++ )
		{
			if( *it == _effect )
			{
				break;
			}
		}
		
		effect * temp = m_effects[i - 1];
		m_effects[i - 1] = _effect;
		m_effects[i] = temp;	
	}
}




bool effectChain::processAudioBuffer( sampleFrame * _buf, const fpp_t _frames )
{
	if( m_enabledModel.value() == FALSE )
	{
		return( FALSE );
	}
	bool more_effects = FALSE;
	for( effectList::iterator it = m_effects.begin(); 
						it != m_effects.end(); ++it )
	{
		more_effects |= ( *it )->processAudioBuffer( _buf, _frames );
#ifdef LMMS_DEBUG
		for( int f = 0; f < _frames; ++f )
		{
			if( fabs( _buf[f][0] ) > 5 || fabs( _buf[f][1] ) > 5 )
			{
				it = m_effects.end()-1;
				printf( "numerical overflow after processing "
					"plugin \"%s\"\n", ( *it )->
					publicName().toAscii().constData() );
				break;
			}
		}
#endif
	}
	return( more_effects );
}




void effectChain::startRunning( void )
{
	if( m_enabledModel.value() == FALSE )
	{
		return;
	}
	
	for( effectList::iterator it = m_effects.begin(); 
						it != m_effects.end(); it++ )
	{
		( *it )->startRunning();
	}
}




bool effectChain::isRunning( void )
{
	if( m_enabledModel.value() == FALSE )
	{
		return( FALSE );
	}
	
	bool running = FALSE;
	
	for( effectList::iterator it = m_effects.begin(); 
				it != m_effects.end() || !running; it++ )
	{
		running = ( *it )->isRunning() && running;
	}
	return( running );
}




void effectChain::clear( void )
{
	for( int i = 0; i < m_effects.count(); ++i )
	{
		delete m_effects[i];
	}
	m_effects.clear();
}


#endif
