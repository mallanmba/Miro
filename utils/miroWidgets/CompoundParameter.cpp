// -*- c++ -*- ///////////////////////////////////////////////////////////////
//
// This file is part of Miro (The Middleware for Robots)
// Copyright (C) 1999-2013
// Department of Neural Information Processing, University of Ulm
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

// This module
#include "CompoundParameter.h"
// This application
#include "SimpleParameter.h"
#include "ParameterList.h"
#include "ParameterDialog.h"
#include "ConfigFile.h"
#include "params/Type.h"
#include "params/Generator.h"
#include "miro/Exception.h"
// The Qt library
#include <QTreeWidget>
// The C++ Standard Library
#include <cassert>


CompoundParameter::CompoundParameter(Miro::CFG::Type const& _type,
				     QDomNode const& _node,
				     QTreeWidgetItem * _parentItem,
				     QTreeWidgetItem * _pre,
				     QObject * _parent,
				     const char * _name) :
  Super(_node, _parentItem, _pre, _parent, _name),
  type_(_type)
{
  if (treeWidgetItem()->treeWidget()->columnCount() >= 3)
  {
    treeWidgetItem()->setText(2, type_.fullName());
  }
}

CompoundParameter::CompoundParameter(Miro::CFG::Type const& _type,
				     QDomNode const& _node,
				     QTreeWidget * _list,
				     QTreeWidgetItem * _pre,
				     QObject * _parent,
				     const char * _name) :
  Super(_node, _list, _pre, _parent, _name),
  type_(_type)
{
  if (treeWidgetItem()->treeWidget()->columnCount() >= 3)
  {
    treeWidgetItem()->setText(2, type_.fullName());
  }
}

void
CompoundParameter::init()
{
  // get complete parameter set including super classes
  Miro::CFG::ParameterVector params =
    config_->description().getFullParameterSet(type_);

  QDomNode n = node().firstChild();
  QTreeWidgetItem * pre = NULL;
  while (!n.isNull()) {
    QDomElement e = n.toElement();
    if (!e.isNull() &&
	e.tagName() == ParameterXML::XML_TAG) {
      ParameterXML * newParam = NULL;
      if (!e.hasAttribute(ParameterXML::XML_ATTRIBUTE_KEY))
	throw Miro::Exception(QString("Parameter tag without name in compound (" +
				      type_.fullName() + ") " + name()));

      QString p = e.attribute(ParameterXML::XML_ATTRIBUTE_KEY);
      QString pLower = p;

      // we need a lower case first letter version for comparison
      pLower[0] = p[0].lower();
      Miro::CFG::ParameterVector::const_iterator i, last = params.end();
      for (i = params.begin(); i != last; ++i) {
	if (i->name_ == pLower) {
	  break;
	}
      }
      if (i == last)
	throw Miro::Exception(QString("Parameter " + p + " unknown in compound (" + 
				      type_.fullName() + ") " + name()));

      if (SimpleParameter::typeFromName(i->type_) != 
	  SimpleParameter::NONE) {
	QTreeWidgetItem * const  pTreeWidgetItem = treeWidgetItem();
	assert(pTreeWidgetItem != NULL);
	newParam = new SimpleParameter(*i, n, pTreeWidgetItem, pre, this, p);
      }
      else if (ParameterList::typeFromName(i->type_) !=
	       ParameterList::NONE) {
	QTreeWidgetItem * const pTreeWidgetItem = treeWidgetItem();
	assert(pTreeWidgetItem != NULL);
	newParam = new ParameterList(*i, n, pTreeWidgetItem, pre, this, p);
      }
      else {
	Miro::CFG::Type const * const t = 
	  config_->description().getType(i->type_);
	if (t != NULL) {
	  QTreeWidgetItem * const pTreeWidgetItem = treeWidgetItem();
  	  assert(pTreeWidgetItem != NULL);
	  newParam =
	    new CompoundParameter(*t, n, pTreeWidgetItem, pre, this, p);
	}
	else
	  throw Miro::Exception(QString("Type " + i->type_ + " of parameter " + p +
					" unknown in compound (" + 
					type_.fullName() + ") " + name()));
      }
      if (newParam != NULL)
	newParam->init();
    }
    n = n.nextSibling();
  }
}

void
CompoundParameter::setParameters()
{
  ParameterDialog dialog(type_,
			 node_.parentNode(), node_, 
			 NULL, this,
			 NULL, name());
      
  int rc = dialog.exec();
  if (rc == QDialog::Accepted) {
    dialog.setXML();
    if (dialog.modified())
      setModified(true);
  }
}

Miro::CFG::Type const&
CompoundParameter::type(QDomNode const& _node,
			QString const& _attribute)
{
  // Fetch the value of the given attribute from the given QDomNode
  QDomNode n = _node;
  QDomElement e = n.toElement();
  QString typeName = e.attribute(_attribute);

  // Map the attribute's value onto a Miro::CFG::Type
  Miro::CFG::Type const * const parameterType =
    ConfigFile::instance()->description().getType(typeName);
  
  if (parameterType == NULL) {
    throw Miro::Exception(QString("Parameter description for " + typeName +
				  " not found.\nCheck whether the relevant description file is loaded (5)."));
  }
  return *parameterType;
}
