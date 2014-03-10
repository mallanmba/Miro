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
// Enable migration from Qt v3 to Qt v4
// #define LSB_Q3SCROLLVIEW

#include "ParameterDialog.h"
#include "SimpleParameter.h"
#include "SimpleParameterEdit.h"
#include "DeferredParameterEdit.h"

#include "ConfigFile.h"

#include "params/Type.h"
#include "params/Generator.h"

#include <qstring.h>
#include <QGroupBox>
#include <qlayout.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qmessagebox.h>
#ifdef LSB_Q3SCROLLVIEW
#include <QScrollArea>
#else
#include <q3scrollview.h>
#endif
#include <qobject.h>
#include <QGridLayout>
#include <QFrame>

#include <cassert>

ParameterDialog::ParameterDialog(Miro::CFG::Type const& _parameterType,
				 QDomNode const& _parentNode, 
				 QDomNode const& _node,
				 ItemXML * _parentItem,
				 ItemXML * _item,
				 QWidget * _parent, const char * _name) :
  Super(_parentNode, _node, _parentItem, _item, _parent, _name),
  config_(ConfigFile::instance()),
  parameterType_(_parameterType),
  params_(config_->description().getFullParameterSet(parameterType_))
{
  assert(!_node.isNull());

  initDialog();

  QSize s = frame_->sizeHint();

  if (s.width() > 800 || 
      s.height() > 600) {

    delete frame_;
#ifdef LSB_Q3SCROLLVIEW
    QWidget * const pScrollAreaParent = groupBox_;
    QScrollArea * const sv = new QScrollArea(pScrollAreaParent);
    assert(sv != NULL);
#else
    Q3ScrollView * sv = new Q3ScrollView(groupBox_, "scrollview");
#endif
    QWidget * const pFrameParent = sv->viewport();
    frame_ = new QFrame(pFrameParent);
#ifdef LSB_Q3SCROLLVIEW
    sv->setWidget(frame_);
    /// @todo What is the counterpart of setting the resize policy?
#else
    sv->addChild(frame_);
    sv->setResizePolicy(Q3ScrollView::AutoOneFit);
#endif
    
    initDialog();

    frame_->sizeHint();
  }

  groupBox_->sizeHint(); // we seem to need this to get the right size (hutz)
  groupBox_->setTitle("Parameters");
}

void
ParameterDialog::setXML()
{
  setModified(false);
  EditVector::const_iterator first, last = editFelds_.end();
  for (first = editFelds_.begin(); first != last; ++first) {
    if ((*first)->modified()) {
      (*first)->setXML();
      setModified(true);
    }
  }
}

void
ParameterDialog::initDialog()
{
  editFelds_.clear();

  // The GridLayout is a child of the ScrollView's viewport
  QWidget * const pGridLayoutParent = frame_;
  QGridLayout * const gridLayout = new QGridLayout(pGridLayoutParent);
  assert(gridLayout != 0);
  // Specify the margin but not the number of rows (params_.size(),
  // columns (3)
  const int margin = 5;
  gridLayout->setContentsMargins(margin, margin, margin, margin);
  const int spacing = 5;
  gridLayout->setSpacing(spacing);

  // add parameter structs:
  unsigned long i = 0;
  Miro::CFG::ParameterVector::const_iterator first, last = params_.end();
  for (first = params_.begin(); first != last; ++first, ++i) {

    // name
    QLabel * name = new QLabel(frame_);
    QString n = first->name_;
    n[0] = n[0].upper();
    name->setText(n);
    gridLayout->addWidget(name, i, 0);


    // search existing entry

    QDomNode parameterNode;
    if (!node_.isNull()) {
      parameterNode = node_.firstChild();
      while(!parameterNode.isNull()) {
	QDomElement e = parameterNode.toElement();
	if (!e.isNull() &&
	    e.attribute("name") == n)
	  break;
	parameterNode = parameterNode.nextSibling();
      }
    }

    // if there is an existing entry
    // and we know our listview,
    // get the corresponding listview item

    ItemXML * childItem = NULL;
    if (!parameterNode.isNull() &&
	item_ != NULL) {
      if (!item_->children().isEmpty()) {
	QObjectList childList = item_->children();
        QListIterator<QObject*> it(childList);
        QObject * c;
        while (it.hasNext()) {
          c = it.next();
	  childItem = dynamic_cast<ItemXML *>(c);
	  assert(childItem != NULL);

	  if (childItem->node().toElement().attribute("name") == n) {
	    break;
	  }
	  childItem = NULL;
	}
      }
    }

    // create the dialog

    ParameterEdit * value;
    SimpleParameter::Type editType =
      SimpleParameter::typeFromName(first->type_);
    if (editType != SimpleParameter::NONE) {
      value = new SimpleParameterEdit(editType,
				      *first,
				      node_,
				      parameterNode,
				      item_,
				      childItem,
				      frame_,
				      n.latin1());

      // add measure
      QLabel * measure = new QLabel(frame_);
      if (!first->measure_.isEmpty()) {
	measure->setText(first->measure_);
	QToolTip::add(measure, (first->type_ != "angle")? first->type_ : QString("double"));
      }
      else
	measure->setText(first->type_);
      gridLayout->addWidget(measure, i, 2);
    }
    else {
      DeferredParameterEdit::EditType deferredEditType =
	DeferredParameterEdit::editType(first->type_);

      value = new DeferredParameterEdit(deferredEditType,
					*first,
					node_,
					parameterNode,
					item_,
					childItem,
					frame_,
					n.latin1());
    }

    gridLayout->addWidget(value->editWidget(), i, 1);
    editFelds_.push_back(value);

  }
}
