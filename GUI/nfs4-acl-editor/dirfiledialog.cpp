/*  Copyright (c) 2006 The Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  David M. Richter <richterd@citi.umich.edu>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the University nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Very cool, free-for-non-commercial-use icons courtesy of
 *  http://themes.freshmeat.net/projects/crystalicons/
 */

#include "dirfiledialog.h"

/* a simple file dialog that will show hidden files and allow you to select
 * a file or a directory (curious shortcomings in the new QFileDialog..)
 */
DirFileDialog::DirFileDialog(QWidget *parent) : QDialog(parent)
{
	dir.setFilter(QDir::AllDirs | QDir::Hidden | QDir::Files);
	dir.setSorting(QDir::DirsFirst);

	isDouble = 0;
	gl = new QGridLayout();
	gl->setSpacing(0);

	l = new QLabel("Looking in:  ");
	l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	gl->addWidget(l, 0, 0, 1, 1);

	cb = new QComboBox();
	cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	cb->setEditable(true);
	cb->setDuplicatesEnabled(false);

	cle = new QLineEdit(cb);
//	//connect(cle, SIGNAL(textChanged(QString)), this, SLOT(<dirAutoComplete>))
//	//cb->setInsertPolicy()
//	//cb->setAutoCompletion()
	cb->setLineEdit(cle);
	connect(cb, SIGNAL(activated(int)), this, SLOT(burrow(int)));
	connect(cle, SIGNAL(returnPressed()), this, SLOT(burrow()));
	gl->addWidget(cb, 0, 1, 1, 4);


	/* add a spacer */
	f = new QFrame();
	f->setMidLineWidth(1);
	f->setLineWidth(2);
	f->setMinimumHeight(2);
	f->setMaximumHeight(4);
	f->setFrameStyle(QFrame::HLine | QFrame::Raised);
	//gl->addWidget(f, 1, 0, 1, 5, Qt::AlignTop);
	gl->addWidget(f, 1, 0, 1, 5);
	gl->setRowMinimumHeight(1, 20);

	lw = new QListWidget();
	//lw->setResizeMode(QListView::Adjust);
	//lw->setUniformItemSizes(true);
	//lw->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	lw->setWrapping(true);
	lw->setSpacing(1);
	connect(lw, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(handleClicked(QListWidgetItem *)));
	connect(lw, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(handleActivated(QListWidgetItem *)));
	connect(lw, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(handleDoubleClicked(QListWidgetItem *)));
	gl->addWidget(lw, 2, 0, 1, 5);


	l = new QLabel("Get NFSv4 ACL for: ");
	l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	gl->addWidget(l, 3, 0, 1, 2, Qt::AlignBottom);

	le = new QLineEdit(this);
	//connect(le, SIGNAL(returnPressed()), this, SLOT(weOut()));
	gl->addWidget(le, 3, 2, 1, 3, Qt::AlignBottom);
	gl->setRowMinimumHeight(3, 40);

	bw = new QWidget(this);
	hl = new QHBoxLayout();
	pb = new QPushButton("OK", bw);
	pb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(pb, SIGNAL(clicked()), this, SLOT(tenFourGoodBuddy()));
	hl->addWidget(pb);

	pb = new QPushButton("Cancel", bw);
	pb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(pb, SIGNAL(clicked()), this, SLOT(negatoryGoodBuddy()));
	hl->addWidget(pb);
	bw->setLayout(hl);
	gl->addWidget(bw, 4, 3, 1, 2, Qt::AlignRight);

	setModal(true);
	setLayout(gl);
	setWindowTitle("Get NFSv4 ACL for ...");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	resize(600, 360);
}

void DirFileDialog::populateCombo()
{
	QString path = dir.absolutePath();

	cb->clear();
	for (;;) {
		cb->addItem(path);
		if (!path.compare(QDir::rootPath()))
			break;
		path = QDir::cleanPath(path.append("/.."));
	}
}	

void DirFileDialog::populateView()
{
	QFileInfoList fil = dir.entryInfoList();
	QFileInfo fi;

	lw_clear(lw);
	for (int i = 0; i < fil.size(); i++) {
		fi = fil.at(i);
		lw->addItem(lwi_new(iconForInfo(fi), fi.fileName()));
	}
	updateGeometry();
}

/* er.. i had several directories with 5000 files in them and it was 
 * intolerably slow each time i used the dialog; this buffer pool fixed it.
 */
QListWidgetItem* DirFileDialog::lwi_new(QIcon icon, QString text)
{
	QListWidgetItem *lwi;

	if (lwi_pool.empty())
		lwi = new QListWidgetItem();
	else
		lwi = lwi_pool.takeFirst();

	lwi->setIcon(icon);
	lwi->setText(text);

	return lwi;
}

/* XXX: #define-ify */
void DirFileDialog::lwi_free(QListWidgetItem *lwi)
{
	if (lwi)
		lwi_pool.append(lwi);
}

void DirFileDialog::lw_clear(QListWidget *qlw)
{
	QListWidgetItem *lwi;

	while ((lwi = qlw->takeItem(0)))
		lwi_free(lwi);
}

void DirFileDialog::burrow(QString path)
{
	//printf("burrowing to: %s\n", qPrintable(path));
	dir.cd(path);
	populateCombo();
	populateView();
}

void DirFileDialog::burrow(int comboIndex)
{
	if (comboIndex == 0)
		return;

	for (int i = 0; i < comboIndex; i++)
		cb->removeItem(0);
	burrow(cb->itemText(0));
}

void DirFileDialog::burrow()
{
	if (dir.absolutePath().compare(cle->text()))
		burrow(cle->text());
}

int DirFileDialog::exec(QString filename)
{
	le->clear();
	burrow(filename);

	return ((QDialog *)this)->exec();
}

void DirFileDialog::tenFourGoodBuddy()
{
	path = le->text();
	done(0);
}

void DirFileDialog::negatoryGoodBuddy()
{
	path = dir.absolutePath();
	done(1);
}

QString DirFileDialog::selectedPath()
{
	if (!path.startsWith("/"))
		return (dir.absolutePath().append("/").append(path));

	return QString(path);
}

void DirFileDialog::handleClicked(QListWidgetItem *item)
{
	if (isDouble) {
		isDouble = 0;
		return; 
	}

	//le->setText(dir.absolutePath().append("/").append(item->text()));
	le->setText(item->text());
}

void DirFileDialog::handleActivated(QListWidgetItem *item)
{
	handleClicked(item);
}

void DirFileDialog::handleDoubleClicked(QListWidgetItem *item)
{
	QString absName = dir.absolutePath().append("/").append(item->text());

	isDouble = 1;
	if (QFileInfo(absName).isDir())
		burrow(absName);
}

void DirFileDialog::keyPressEvent(QKeyEvent *e)
{
	int key = e->key();

	//if ((e->modifiers() & Qt::ControlModifier) && (e->type() & QEvent::KeyPress)) {
	if (key == Qt::Key_Escape) {
		e->accept();
		negatoryGoodBuddy();
	} else if (key == Qt::Key_Enter || key == Qt::Key_Return) {
		e->accept();
		tenFourGoodBuddy();
	}
}

QIcon DirFileDialog::iconForInfo(QFileInfo &info)
{
	/* XXX: need to expand icons for, e.g., symlinks, etc */
	if (info.isDir())
		return (QIcon(QString(":images/dir.png")));
	else
		return (QIcon(QString(":images/file.png")));
}
