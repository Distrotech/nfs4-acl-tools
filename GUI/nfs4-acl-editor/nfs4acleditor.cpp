/*  Copyright (c) 2006 The Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  David M. Richter <richterd@citi.umich.edu>
 *  Alexis Mackenzie <allamack@citi.umich.edu>
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
 */

#include <string.h>
#include "nfs4acleditor.h"

NFS4_ACL_Editor::NFS4_ACL_Editor(QWidget *parent, char *filearg) : QMainWindow(parent)
{
	uis = QString(":images/user.gif");
	gis = QString(":images/group2.gif");
	eis = QString(":images/everyone2.gif");

	uils = QString(":images/user-light.gif");
	gils = QString(":images/group2.gif");
	eils = QString(":images/everyone2.gif");

	nacl = NULL;
	pbg = NULL;
	dpbg = NULL;
	fpbg = NULL;
	selectedACE = NULL;
	selectedLogical = -1;
	selectedVisual = -1;
	is_directory = 0;
	deleting = 0;

	setWindowIcon(QIcon(QPixmap(":images/citi-icon.gif")));
	populateMenus();

	cw = new QWidget(this);
	setCentralWidget(cw);

	sw = new QStackedWidget();
	sw->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	gl = new QGridLayout(cw);
	fl = new QLabel(" ");
	fl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	gl->addWidget(fl, 0, 0, 1, 5);

	/* the main ACL table */
	sl << "ACE type" << "Principal" << "Permissions" << "Flags";
	tw  = new QTableWidget(0, 4);
	tw->setHorizontalHeaderLabels(sl);
	tw->setAlternatingRowColors(true);
	tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tw->setSelectionMode(QAbstractItemView::SingleSelection);
	tw->setSelectionBehavior(QAbstractItemView::SelectRows);
	tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tw->setFocus(Qt::OtherFocusReason);
	tw->setSortingEnabled(false);
	tw->setColumnWidth(2, 130);
	tw->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	tw->horizontalHeader()->setMovable(true);
	tw->verticalHeader()->setResizeMode(QHeaderView::Custom);

	//tw->setDragEnabled(true);
	//tw->setAcceptDrops(true);
	//tw->setDropIndicatorShown(true);
	/// should be fixed by Trolltech///	tw->verticalHeader()->setMovable(true);
	///	//connect(tw->verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(aceMoved(int, int, int)));
	///	//connect(tw, SIGNAL(rowMoved(int, int, int)), this, SLOT(aceMoved(int, int, int)));

	connect(tw, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(aceSelected(int, int, int, int)));
	gl->addWidget(tw, 1, 0, 2, 5);
	gl->setRowStretch(1, 5);
	gl->setColumnStretch(1, 50);

	/* add a spacer */
	f = new QFrame();
	f->setMidLineWidth(1);
	f->setLineWidth(2);
	f->setMinimumHeight(2);
	f->setMaximumHeight(4);
	f->setFrameStyle(QFrame::HLine | QFrame::Raised);
	gl->addWidget(f, 3, 0, 1, 5, Qt::AlignTop);
	gl->setRowMinimumHeight(3, 15);
	//gl->setColumnMinimumWidth(2, 15);

	/* most of the fields underneath the ACL table */
	populateTypeWho();
	populatePerms();
	populateFlags();
	populateButtons();

	cw->setLayout(gl);
	tw->resizeColumnToContents(0);
	initWithFile(QString(filearg));

#ifdef DEBUG
	for (int i = 0; i < acl.size(); i++) {
		ace = acl.at(i);
		dprintf("%d) i see %x is '%s' in %x, length %d\n", i, ace, ace->who, ace->who, strlen(ace->who));
	}
#endif
	normal = QPalette(tw->palette());
	light = QPalette(normal);
	qc = normal.color(QPalette::Highlight);
	light.setColor(QPalette::Highlight, qc.light(125));
}


void NFS4_ACL_Editor::typeModified(int index)
{
	int newtypemask = maskForType(index);

	if (acl.empty() || selectedLogical < 0 || selectedVisual < 0 || selectedACE == NULL)
		return;

	selectedACE->type = newtypemask;
	setRowType(selectedLogical, textForType(index));
	setRowIcon(selectedLogical, iconForACE(selectedACE));
}

void NFS4_ACL_Editor::principalModified(const QString &s)
{
//	printf("ICON FOR PRINCIPAL: %s (flags: %x) (item: %p) (logical: %d) (count: %d)\n", qPrintable(s), selectedACE->flag, tw->item(selectedLogical, 1), selectedLogical, tw->rowCount());

	setRowPrincipal(selectedLogical, s);
	if (isSpecialGroup(s)) {
		groupifyACE(selectedACE);
		updateFlags();
	} else if (isEveryone(s)) {
		ungroupifyACE(selectedACE);
		updateFlags();
	}
	syncWho(selectedACE, qPrintable(s));
	setRowIcon(selectedLogical, iconForPrincipal(qPrintable(s), isGroupACE(selectedACE)));
}

void NFS4_ACL_Editor::permsModified(QAbstractButton *a)
{
	QCheckBox *c = (QCheckBox *)a;

	if (acl.empty() || selectedLogical < 0 || selectedVisual < 0 || selectedACE == NULL)
		return;

	if (c->isChecked())
		selectedACE->access_mask |= pbg->id(c);
	else
		selectedACE->access_mask ^= pbg->id(c);
	updatePerms();
}

void NFS4_ACL_Editor::shortcutsModified(QAbstractButton *a)
{
	if (acl.empty() || selectedLogical < 0 || selectedVisual < 0 || selectedACE == NULL)
		return;

	selectedACE->access_mask = sbg->id(a);
	updatePerms();
}

void NFS4_ACL_Editor::flagsModified(QAbstractButton *a)
{
	QCheckBox *c = (QCheckBox *)a;

	if (acl.empty() || selectedLogical < 0 || selectedVisual < 0 || selectedACE == NULL)
		return;

	if (c->isChecked())
		selectedACE->flag |= fbg->id(c);
	else
		selectedACE->flag ^= fbg->id(c);

	updateFlags();
	setRowIcon(selectedLogical, iconForACE(selectedACE));
}

void NFS4_ACL_Editor::aceSelected(int row, int column, int prevrow, int prevcolumn)
{
	if (acl.empty())
		return;

	dprintf("aceSelected(): deleting? %d   row %d col %d  prevrow %d prevcol %d   currentrow %d\n", deleting, row, column, prevrow, prevcolumn, tw->currentRow());
	if (!deleting) {
		selectedLogical = row;
		selectedVisual = tw->visualRow(selectedLogical);
	}
	else {
		/* special case: if you're deleting the nth row, 'row' is n and 'prevrow' is n+1,
		 *       -but-   if you're deleting the 0th row, 'row' is 1 and 'prevrow' is 0
		 *
		 * this can be tricky: means that if you're on the 1st row and delete it, current
		 * becomes 0.  but if you're on the 0th row and delete IT, current becomes 1!
		 */
		if (row == 1 && prevrow == 0) {
			selectedLogical = 0;
			selectedVisual = 0;
		}

		/* during a DELETE, the outgoing row is still included in rowCount().
		 * currentRow is the new end-of-list index.
		 * to see if currentRow will-soon-be the end-of-list, subtract two (1 for 0-based,
		 *   and 1 for "haven't decremented for the outgoing row yet"-in the table).
		 */
		else if (tw->currentRow() == tw->rowCount() - 2) {
			selectedLogical = tw->currentRow();
			selectedVisual = selectedLogical;
		}

		else {
			if (selectedLogical != 0) {
				selectedLogical = row + 1;
				if (selectedLogical >= tw->rowCount())
					selectedLogical = tw->currentRow();
			} else
				selectedLogical = row;

			selectedVisual = tw->visualRow(selectedLogical);
		}
	}

	dprintf("aceSelected(): NEAR-END:  logical %d  visual %d  currentrow %d  aces %d  rows %d\n", selectedLogical, selectedVisual, tw->currentRow(), acl.size(), tw->rowCount());
	if (selectedLogical < 0 || selectedVisual < 0 || row < 0) {
		dprintf("aceSelected(): selectedLogical %d, selectedVisual %d, ACE %p, ACL supposedly has %d ACEs\n", selectedLogical, selectedVisual, selectedACE, acl.size());
		selectedACE = NULL;
		return;
	}
	selectedACE = acl.at(selectedVisual);
	le->setText(selectedACE->who);
	updateType();
	updatePerms();
	updateFlags();

	dprintf("aceSelected(): sindex %d is type %d '%s'\n", selectedVisual, selectedACE->type, selectedACE->who);
}

void NFS4_ACL_Editor::aceMoved(int logical, int oldind, int newind)
{
	acl.move(oldind, newind);
	selectedLogical = logical;
	selectedVisual = tw->visualRow(selectedLogical);
	selectedACE = acl.at(selectedVisual);
	dprintf("aceMoved(): selectedLogical is %d == logical is %d, visual is %d == newind %d\n", selectedLogical, logical, selectedVisual, newind);
}

void NFS4_ACL_Editor::updateType()
{
	for (int i = 0; i < TYPE_COUNT; i++) {
		if (maskForType(i) == selectedACE->type) {
			cmb->setCurrentIndex(i);
			break;
		}
	}
}

void NFS4_ACL_Editor::updatePerms()
{
	dprintf("UPDATE_PERMS: currentRow: %d; selectedLogical: %d; selectedVisual: %d  pbg: %p  sACE %d %p %s mask %x  || ", tw->currentRow(), selectedLogical, selectedVisual, pbg, selectedACE->type, selectedACE, selectedACE->who, selectedACE->access_mask );
	updateCheckBoxesWithMask(pbg, selectedACE->access_mask);
	setRowPerms(selectedLogical, nfs4_get_ace_access(selectedACE, buf, is_directory));
}

void NFS4_ACL_Editor::updateFlags()
{
	updateCheckBoxesWithMask(fbg, selectedACE->flag);
	setRowFlags(selectedLogical, nfs4_get_ace_flags(selectedACE, buf));
}

void NFS4_ACL_Editor::updateCheckBoxesWithMask(QButtonGroup *bg, unsigned int mask)
{
	if (bg == NULL)
		return;

	ButtonList bl = (ButtonList)bg->buttons();
	QCheckBox *c;

	for (int i = 0; i < bl.size(); i++) {
		c = (QCheckBox *)bl.at(i);
		if (bg->id(c) & mask)
			c->setCheckState(Qt::Checked);
		else
			c->setCheckState(Qt::Unchecked);
	}
}

void NFS4_ACL_Editor::getACL()
{
	doHighlight(0);
	if (!dfd->exec(filename))
		initWithFile(dfd->selectedPath());
}

void NFS4_ACL_Editor::reloadACL()
{
	nfs4_free_acl(nacl);
	acl.clear();
	initWithFile(filename);
}

void NFS4_ACL_Editor::newACE()
{
	struct nfs4_ace *ace;

	if (nacl == NULL)
		return;

	ace = nfs4_new_ace(nacl->is_directory, NFS4_ACE_ACCESS_ALLOWED_ACE_TYPE, 0, 0, NFS4_ACL_WHO_NAMED, " ");
	if (!ace) {
		fprintf(stderr, "ERROR: failed to create new ACE (%p).\n", nacl);
		perror(strerror(errno));
		return;
	}

	/* trim the name to zero-length.  leaks a byte --
	 * it's like spilling some malt liquor for fallen homies. */
	ace->who[0] = '\0';

	if (nfs4_prepend_ace(nacl, ace)) {
		fprintf(stderr, "ERROR: failed to add ACE to ACL.\n");
		free(ace);
		return;
	}
	acl.prepend(ace);
	selectedLogical = 0;
	tw->insertRow(selectedLogical);
	selectedVisual = tw->visualRow(selectedLogical);
	selectedACE = ace;

	dprintf("newACE(): currentRow: %d; selectedLogical: %d == selectedVisual: %d == acl->size(%d) - 1\n", tw->currentRow(), selectedLogical, selectedVisual, acl.size());

	initRowWithACE(selectedLogical, selectedACE);
	tw->setCurrentCell(selectedLogical, 0);
}

void NFS4_ACL_Editor::deleteACE()
{
	if (acl.empty() || nacl == NULL || selectedLogical < 0 || selectedVisual < 0 || selectedACE == NULL)
		return;
//	dprintf("deleteACE(): deleting %s  type %d   logical: %d  visual: %d  (ACE: %x %s)\n", selectedACE->who, selectedACE->type, selectedLogical, selectedVisual, acl.at(selectedVisual)->type, acl.at(selectedVisual)->who);

	deleting = 1;
	dprintf("deleteACE(): naces %d  acl %d  rows %d \n", nacl->naces, acl.size(), tw->rowCount());
	dprintf("deleteACE(): sACE %p  %s  ACL(0) %p  %s\n", selectedACE, selectedACE->who, acl.at(0), acl.at(0)->who);
	nfs4_remove_ace(nacl, selectedACE);
	acl.removeAt(tw->currentRow());

	dprintf("\n\n");
	for (int z = 0; z < tw->rowCount(); z++) {
		dprintf("logical row %d seems to be visual row %d\n", z, tw->visualRow(z));
	}
	tw->removeRow(tw->currentRow());
	deleting = 0;

	if (tw->rowCount() > 0) {
		dprintf("deleteACE(): post-removeRow:  logical %d  visual %d   currentRow %d  currentVis %d  ACL size %d\n", selectedLogical, selectedVisual, tw->currentRow(), tw->visualRow(tw->currentRow()), acl.size());
		// set in aceSelected instead...	selectedLogical = tw->currentRow();
		selectedACE = acl.at(selectedLogical);
	} else {
		selectedLogical = -1;
		selectedVisual = -1;
		selectedACE = NULL;
	}
	tw->setCurrentCell(selectedLogical, 0);
}

void NFS4_ACL_Editor::setACL()
{
	struct nfs4_ace *ace;

	if (acl.empty() || nacl == NULL)
		return;

	/* must reorder nfs4_acl to match our NFS4_ACL and make "who" match */
	//printf("\npre-reordering:\n");	//acl_nfs4_print(nacl);
	TAILQ_INIT(&nacl->ace_head);
	for (int i = 0; i < acl.size(); i++) {
		ace = acl.at(i);
		TAILQ_INSERT_TAIL(&nacl->ace_head, ace, l_ace);
		syncWho(ace, qPrintable(tw->item(i, 1)->text()));
	}
	//printf("\npost-reordering:\n");	//acl_nfs4_print(nacl);
	if (nfs4_set_acl(nacl, qPrintable(filename)))
		fprintf(stderr, "ERROR: failed to set ACL \n");
}

/* b/c of signals, moveUp/moveDown need to be functions and not just #defines */
void NFS4_ACL_Editor::move(int by)
{
	int at = tw->currentRow();
	int to = at + by;

	swapRows(at, to);
	tw->selectRow(to);
}

void NFS4_ACL_Editor::moveUp()
{
	    move(-1);
}

void NFS4_ACL_Editor::moveDown()
{
	    move(1);
}

void NFS4_ACL_Editor::initWithFile(QString file)
{
	filename.clear();
	if (file.isEmpty())
		goto out;

	dprintf("\nINIT: STARTING\n");
	nacl = nfs4_acl_for_path(qPrintable(file));
	if (nacl == NULL) {
		filename.append("<font color='#a01020'>ERROR: file not found, or perhaps it's not on an NFSv4 mount.</font>");
		goto out;
	}
	filename.append(file);

	is_directory = nacl->is_directory;
	pbg = (is_directory) ? dpbg : fpbg;
	sbg = (is_directory) ? dsbg : fsbg;

	acl.clear();
	nfs4_acl_to_NFS4_ACL(nacl, &acl);
	sw->setCurrentIndex(is_directory);

	dprintf("INIT: .. about to remove rows..\n");
	deleting = 1;
	while (tw->rowCount() > 0)
		tw->removeRow(0);
	deleting = 0;

	dprintf("INIT: .. about to insert rows..\n");
	for (int i = 0; i < acl.size(); i++) {
		ace = acl.at(i);
		//nfs4_print_ace(ace, is_directory);
		tw->insertRow(tw->rowCount());
		initRowWithACE(tw->rowCount() - 1, ace);
	}
	tw->setCurrentCell(0, 0);
out:
	reformatFilename();
}

void NFS4_ACL_Editor::initRowWithACE(int row, struct nfs4_ace *ace)
{
	tw->setVerticalHeaderItem(row, new QTableWidgetItem());
	setRowIcon(row, iconForACE(ace));
	tw->setItem(row, 0, new QTableWidgetItem(nfs4_get_ace_type(ace, buf, 1)));
	tw->setItem(row, 1, new QTableWidgetItem(ace->who));
	tw->setItem(row, 2, new QTableWidgetItem(nfs4_get_ace_access(ace, buf, is_directory)));
	tw->setItem(row, 3, new QTableWidgetItem(nfs4_get_ace_flags(ace, buf)));
}

void NFS4_ACL_Editor::initRowWithList(int row, QStringList *t)
{
	if (!t || t->size() != 4) {
		fprintf(stderr, "ERROR: initRowWithList() bailed (t: %p)\n", t);
		exit(1);
	}
	for (int i = 0; i < 4; i++)
		tw->item(row, i)->setText(t->at(i));
}

QStringList* NFS4_ACL_Editor::listForRow(int row)
{
	QStringList *qsl = new QStringList();

	for (int i = 0; i < 4; i++)
		qsl->append(tw->item(row, i)->text());

	return qsl;
}

void NFS4_ACL_Editor::swapRows(int from, int to)
{
	QStringList *lf, *lt;
	QIcon fip, tip;

	if (to < 0 || to > tw->rowCount() - 1)
		return;

	dprintf("swapRows(): from %d  to %d\n", from, to);
	fip = iconForRow(from);
	tip = iconForRow(to);
	lf = listForRow(from);
	lt = listForRow(to);
	initRowWithList(to, lf);
	initRowWithList(from, lt);
	setRowIcon(to, QIcon(fip));
	setRowIcon(from, QIcon(tip));
	acl.swap(from, to);

	delete lf;
	delete lt;
}

QIcon NFS4_ACL_Editor::iconForRow(int row)
{
	return tw->verticalHeaderItem(row)->icon();
}

QIcon NFS4_ACL_Editor::iconForACE(struct nfs4_ace *ace)
{
	return iconForACE(ace, 0);
}

QIcon NFS4_ACL_Editor::iconForACE(struct nfs4_ace *ace, int highlight)
{
	if (ace == NULL)
		return QIcon();

	dprintf("ICON FOR  '%s'  type '%d'  flags '%x'\n", ace->who, ace->whotype, ace->flag);
	if (ace->whotype == NFS4_ACL_WHO_GROUP || (ace->flag & NFS4_ACE_IDENTIFIER_GROUP))
		return QIcon(highlight ? gils : gis);
	else if (ace->whotype == NFS4_ACL_WHO_NAMED || ace->whotype == NFS4_ACL_WHO_OWNER)
		return QIcon(highlight ? uils : uis);
	else if (ace->whotype == NFS4_ACL_WHO_EVERYONE)
		return QIcon(highlight ? eils : eis);
	return QIcon();
}

QIcon NFS4_ACL_Editor::iconForPrincipal(const char *name, int is_group)
{
	if (name == NULL)
		return QIcon();

	if (!strncmp(NFS4_ACL_WHO_EVERYONE_STRING, name, NFS4_MAX_PRINCIPALSIZE))
		return QIcon(eis);
	else if (!strncmp(NFS4_ACL_WHO_GROUP_STRING, name, NFS4_MAX_PRINCIPALSIZE)
				|| is_group)
		return QIcon(gis);
	else
		return QIcon(uis);
}

void NFS4_ACL_Editor::syncWho(struct nfs4_ace *ace, const char *who)
{
	unsigned int i = 0;

	if (ace == NULL || who == NULL)
		return;

	dprintf("syncWho(): changing %x '%s' (%x) to '%s' (type %d)\n", ace, ace->who, ace->who, who, ace->type);
	if (strncmp(ace->who, who, NFS4_MAX_PRINCIPALSIZE)) {
		i = strlen(who);
		if (i > strlen(ace->who)) {
			dprintf("syncWho(): fromlen %d  tolen %u\n", strlen(ace->who), i);
		}
		strlcpy(ace->who, who, NFS4_MAX_PRINCIPALSIZE);
	}
}

void NFS4_ACL_Editor::reformatFilename()
{
	formattedfilename.clear();
	if (! filename.isEmpty()) {
		formattedfilename.append("<font size=-1>NFSv4 ACL for ");
		if (is_directory)
			formattedfilename.append("directory");
		else
			formattedfilename.append("file");

		formattedfilename.append(":&nbsp;&nbsp;</font><br>");
		formattedfilename.append("<b><font color='#002060'>");
		formattedfilename.append(filename);
	}
	else
		formattedfilename.append("<b><font>(no file or directory selected)");
	formattedfilename.append("</font></b>");
	fl->setText(formattedfilename);
}

void NFS4_ACL_Editor::populateMenus()
{
	fileMenu = menuBar()->addMenu("&File");
	dfd = new DirFileDialog(this);
	a = new QAction("&Get ACL for file/dir ...", this);
	a->setShortcut(QKeySequence(QString("Ctrl+G")));
	connect(a, SIGNAL(triggered()), this, SLOT(getACL()));
	fileMenu->addAction(a);

	a = new QAction("&Reload/Revert ACL", this);
	a->setShortcut(QKeySequence(QString("Ctrl+R")));
	a->setToolTip("Set this tooltip on the button, silly.");
	connect(a, SIGNAL(triggered()), this, SLOT(reloadACL()));
	fileMenu->addAction(a);

	a = new QAction("&Set ACL", this);
	a->setShortcut(QKeySequence(QString("Ctrl+S")));
	a->setToolTip("Set the NFSv4 ACL for the current file or directory");
	connect(a, SIGNAL(triggered()), this, SLOT(setACL()));
	fileMenu->addAction(a);

	fileMenu->addSeparator();
	a = new QAction("&Quit", this);
	a->setShortcut(QKeySequence(QString("Ctrl+Q")));
	a->setToolTip("Quit");
	connect(a, SIGNAL(triggered()), this, SLOT(quit()));
	fileMenu->addAction(a);


	actionsMenu = menuBar()->addMenu("&Actions");
	a = new QAction("&New ACE", this);
	a->setShortcut(QKeySequence(QString("Ctrl+N")));
	a->setToolTip("Create a new ACE in the current ACL");
	connect(a, SIGNAL(triggered()), this, SLOT(newACE()));
	actionsMenu->addAction(a);

	a = new QAction("Delete ACE", this);
	a->setShortcut(QKeySequence(QString("Ctrl+Backspace")));
	a->setToolTip("Delete the selected ACE");
	connect(a, SIGNAL(triggered()), this, SLOT(deleteACE()));
	actionsMenu->addAction(a);

	a = new QAction("Move ACE &up", this);
	a->setShortcut(QKeySequence(QString("Ctrl+Up")));
	connect(a, SIGNAL(triggered()), this, SLOT(moveUp()));
	actionsMenu->addAction(a);

	a = new QAction("Move ACE &down", this);
	a->setShortcut(QKeySequence(QString("Ctrl+Down")));
	connect(a, SIGNAL(triggered()), this, SLOT(moveDown()));
	actionsMenu->addAction(a);
}

void NFS4_ACL_Editor::populateTypeWho()
{
	cmb = new QComboBox(cw);
	cmb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	for (int i = 0; i < TYPE_COUNT; i++)
		cmb->addItem(textForType(i));

	connect(cmb, SIGNAL(activated(int)), this, SLOT(typeModified(int)));
	gl->addWidget(cmb, 4, 0);

	le = new QLineEdit(cw);
	le->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	connect(le, SIGNAL(textChanged(const QString &)), this, SLOT(principalModified(const QString &)));
	gl->addWidget(le, 4, 1, 1, 2);

	gcb = new QCheckBox("Is this a group?", cw);
	gcb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	gl->addWidget(gcb, 4, 3, Qt::AlignLeft);
}

void NFS4_ACL_Editor::populatePerms()
{
	dpgb = new QGroupBox("Permissions");
	fpgb = new QGroupBox("Permissions");
	dpbg = new QButtonGroup();
	fpbg = new QButtonGroup();

	populatePerms(dpgb, dpbg, 1);
	populatePerms(fpgb, fpbg, 0);

	sw->addWidget(fpgb);
	sw->addWidget(dpgb);
	pbg = fpbg;
	sbg = fsbg;
	gl->addWidget(sw, 5, 0, 3, 2);
}

void NFS4_ACL_Editor::populatePerms(QGroupBox *qgb, QButtonGroup *qbg, int is_dir)
{
	const struct __access_info *info;
	int total_perm_count;

	qbg->setExclusive(false);
	sgl = new QGridLayout(qgb);
	sgl->setSpacing(0);

	if (is_dir) {
		info = __dir_perm_info;
		total_perm_count = DIR_PERM_COUNT;
	} else {
		info = __file_perm_info;
		total_perm_count = FILE_PERM_COUNT;
	}
	for (int i = 0; i < COLUMN1_PERM_COUNT; i++) {
		cb = new QCheckBox(info[i].text);
		qbg->addButton(cb, info[i].mask);
		sgl->addWidget(cb, i, 0, Qt::AlignTop);
	}
	for (int i = COLUMN1_PERM_COUNT, j = 0; i < total_perm_count; i++, j++) {
		cb = new QCheckBox(info[i].text);
		qbg->addButton(cb, info[i].mask);
		sgl->addWidget(cb, j, 1, Qt::AlignTop);
	}
	connect(qbg, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(permsModified(QAbstractButton *)));

	/* add a spacer */
	f = new QFrame(qgb);
	f->setMidLineWidth(1);
	f->setLineWidth(1);
	f->setMinimumHeight(2);
	f->setMaximumHeight(4);
	f->setFrameStyle(QFrame::HLine | QFrame::Raised);
	sgl->addWidget(f, FILE_PERM_COUNT1, 0, 2, 2);
	sgl->setRowMinimumHeight(FILE_PERM_COUNT1, 15);
	sgl->addWidget(new QLabel("<font size=-1><i><b>shortcuts</b></i></font>"), (FILE_PERM_COUNT + 1), 0, 1, 2, Qt::AlignHCenter);

	w = new QWidget(qgb);
	hbl = new QHBoxLayout();
	sbg = new QButtonGroup(cw);
	if (is_dir)
		dsbg = sbg;
	else
		fsbg = sbg;

	for (int i = 0; i < EXTRA_PERM_COUNT; i++) {
		pb = new QPushButton(textForExtraPerm(i), w);
		pb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		sbg->addButton(pb, maskForExtraPerm(i));
		hbl->addWidget(pb);
	}
	hbl->setSpacing(0);
	hbl->setMargin(0);
	w->setLayout(hbl);
	sgl->addWidget(w, (FILE_PERM_COUNT + 2), 0, 2, 2, Qt::AlignBottom);

	connect(sbg, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(shortcutsModified(QAbstractButton *)));
	qgb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	qgb->setLayout(sgl);
}

void NFS4_ACL_Editor::populateFlags()
{
	fgb = new QGroupBox("Inheritance flags", cw);
	fgb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	fbg = new QButtonGroup(cw);
	fbg->setExclusive(false);
	sgl = new QGridLayout(fgb);
	sgl->setSpacing(0);

	for (int i = 0; i < FLAG_COUNT; i++) {
		cb = new QCheckBox(textForFlag(i), fgb);
		fbg->addButton(cb, maskForFlag(i));
		sgl->addWidget(cb, i, 0, Qt::AlignTop);
	}
	fbg->addButton(gcb, NFS4_ACE_IDENTIFIER_GROUP);

	connect(fbg, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(flagsModified(QAbstractButton *)));
	fgb->setLayout(sgl);
	gl->addWidget(fgb, 5, 2, 1, 2, Qt::AlignTop);
}

void NFS4_ACL_Editor::populateButtons()
{
	sb = new QPushButton("&New ACE", cw);
	//sb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(sb, SIGNAL(clicked()), this, SLOT(newACE()));
	gl->addWidget(sb, 6, 2, 1, 1);

	rb = new QPushButton("&Delete ACE", cw);
	//rb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(rb, SIGNAL(clicked()), this, SLOT(deleteACE()));
	gl->addWidget(rb, 7, 2, 1, 1);

	sb = new QPushButton("&Set ACL", cw);
	//sb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(sb, SIGNAL(clicked()), this, SLOT(setACL()));
	gl->addWidget(sb, 6, 3, 1, 1);

	rb = new QPushButton("&Reload ACL", cw);
	//rb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(rb, SIGNAL(clicked()), this, SLOT(reloadACL()));
	gl->addWidget(rb, 7, 3, 1, 1);
}

void NFS4_ACL_Editor::doHighlight(int willDo)
{
	if (tw->rowCount() < 1)
		return;

	tw->setPalette(willDo ? light : normal);
	setRowIcon(selectedLogical, iconForACE(selectedACE, willDo));
}

void NFS4_ACL_Editor::keyPressEvent(QKeyEvent *e)
{
	if ((e->modifiers() & Qt::ControlModifier) && (e->type() & QEvent::KeyPress))
		doHighlight(1);
}

void NFS4_ACL_Editor::keyReleaseEvent(QKeyEvent *e)
{
	if (!(e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Control))
		doHighlight(0);
}

void NFS4_ACL_Editor::activationChange(QEvent *e)
{
	dprintf("activationChange: %x\n", e->type());
	if (e->type() == QEvent::WindowUnblocked) {
		dprintf("unblocked\n");
		setRowIcon(selectedLogical, iconForACE(selectedACE));
	}
}

void NFS4_ACL_Editor::quit()
{
	qApp->quit();
}

/* why does it always squeeze the table view?? */
void NFS4_ACL_Editor::show()
{
	((QMainWindow *)this)->show();
	resize(width(), height() + 20);
//	tw->setIconSize(QSize(48, 48));
//	tw->verticalHeader()->setIconSize(QSize(48, 48));

	dprintf("show()ing!  table w: %d h: %d,  shw: %d  shh: %d,\n"
			"        viewport w: %d h: %d,  shw: %d  shh: %d \n"
			"          window w: %d h: %d,  shw: %d  shh: %d \n",
			tw->width(), tw->height(), tw->sizeHint().width(), tw->sizeHint().height(),
			tw->viewport()->width(), tw->viewport()->height(), tw->viewport()->sizeHint().width(),
			tw->viewport()->sizeHint().height(),
			width(), height(), sizeHint().width(), sizeHint().height());
}
