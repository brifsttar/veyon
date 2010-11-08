/*
 * ConfigurationObject.cpp - implementation of ConfigurationObject
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QStringList>

#include "Configuration/Object.h"
#include "Configuration/LocalStore.h"
#include "Configuration/XmlStore.h"

#include "Logger.h"


namespace Configuration
{


Object::Object( Store::Backend _backend, Store::Scope _scope ) :
	m_store( NULL ),
	m_customStore( false )
{
	switch( _backend )
	{
		case Store::LocalBackend:
			m_store = new LocalStore( _scope );
			break;
		case Store::XmlFile:
			m_store = new XmlStore( _scope );
			break;
		default:
			qCritical( "Invalid Store::Backend %d selected in "
					"Object::Object()", _backend );
			break;
	}

	reloadFromStore();
}




Object::Object( Store *store ) :
	m_store( store ),
	m_customStore( true )
{
	reloadFromStore();
}




Object::~Object()
{
	if( !m_customStore )
	{
		delete m_store;
	}
}



// allow easy merging of two data maps - source is dominant over destination
static Object::DataMap operator+( Object::DataMap dst, Object::DataMap src )
{
	for( Object::DataMap::ConstIterator it = src.begin(); it != src.end(); ++it )
	{
		if( it.value().type() == QVariant::Map && dst.contains( it.key() ) )
		{
			dst[it.key()] = dst[it.key()].toMap() + it.value().toMap();
		}
		else
		{
			dst[it.key()] = it.value();
		}
	}
	return dst;
}




Object &Object::operator+=( const Object &ref )
{
	m_data = m_data + ref.data();

	return *this;
}




QString Object::value( const QString & _key, const QString & _parentKey ) const
{
	// empty parentKey?
	if( _parentKey.isEmpty() )
	{
		// search for key in toplevel data map
		if( m_data.contains( _key ) )
		{
			return m_data[_key].toString();
		}
		return QString();
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = _parentKey.split( '/' );
	DataMap currentMap = m_data;
	foreach( const QString & _level, subLevels )
	{
		if( currentMap.contains( _level ) &&
			currentMap[_level].type() == QVariant::Map )
		{
			currentMap = currentMap[_level].toMap();
		}
		else
		{
			return QString();
		}
	}

	// ok, we're there - does the current submap then contain our key?
	if( currentMap.contains( _key ) )
	{
		return currentMap[_key].toString();
	}
	return QString();
}




void Object::setValue( const QString & key,
			const QString & value,
			const QString & parentKey )
{
	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	QStringList subLevels = parentKey.split( '/' );
	m_data = setValueRecursive( m_data, subLevels, key, value );
}




Object::DataMap Object::setValueRecursive( DataMap data,
									QStringList subLevels,
									const QString &key,
									const QString &value )
{
	if( subLevels.isEmpty() )
	{
		// search for key in toplevel data map
		if( !data.contains( key ) || data[key].type() == QVariant::String )
		{
			data[key] = value;
		}
		else
		{
			qWarning( "cannot replace sub data map with a "
						"string value!" );
		}

		return data;
	}

	const QString level = subLevels.takeFirst();
	if( data.contains( level ) )
	{
		if( !data[level].type() == QVariant::Map )
		{
			qWarning( "parent key points doesn't point to a data map!" );
			return data;
		}
	}
	else
	{
		data[level] = DataMap();
	}

	data[level] = setValueRecursive( data[level].toMap(), subLevels, key, value );

	return data;
}


}

