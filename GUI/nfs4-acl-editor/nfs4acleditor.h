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

#ifndef NFS4_ACL_EDITOR_H
#define NFS4_ACL_EDITOR_H 1

extern "C" {
#include "nfs4.h"
#include "libacl_nfs4.h"
}

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QApplication>
#include <QtCore/qlist.h>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QGridLayout>
#include <QStringList>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QFrame>
#include <QPixmap>
#include <QBitmap>
#include <QMenu>
#include <QMenuBar>
#include <QKeySequence>
#include <QFileDialog>
#include <QMap>
#include <QListWidget>
#include <QStringList>
#include <QKeyEvent>
#include <QPalette>
#include <QColor>
#include <QStackedWidget>
#include <QEvent>

#include "dirfiledialog.h"

//#define DEBUG 1
#ifdef DEBUG
#define dprintf(format, args...) printf(format, ## args)
#else
#define dprintf(format, args...)
#endif

#define isGroupFlag(f) (f & NFS4_ACE_IDENTIFIER_GROUP)
#define isGroupACE(a)  (isGroupFlag(a->flag))
#define isSpecialGroup(s) (!strncmp(qPrintable(s), "GROUP@", XXX_PRINCIPAL_MAX))
#define isEveryone(s) (!strncmp(qPrintable(s), "EVERYONE@", XXX_PRINCIPAL_MAX))
#define groupifyACE(a) a->flag |= NFS4_ACE_IDENTIFIER_GROUP
#define ungroupifyACE(a) a->flag &= ~NFS4_ACE_IDENTIFIER_GROUP


typedef QList<QAbstractButton *> ButtonList;
typedef QList<struct nfs4_ace*> NFS4_ACL;
typedef QListIterator<struct nfs4_ace*> NFS4_ACL_Iterator;

static void nfs4_acl_to_NFS4_ACL(nfs4_acl *acl, NFS4_ACL *newacl)
{
    struct nfs4_ace *ace = nfs4_get_first_ace(acl);

    while (ace != NULL) {
        newacl->append(ace);
        ace = nfs4_get_next_ace(&ace);
    }
}

struct __access_info {
	unsigned long mask;
	char *text;
	char *flag;
};

static const struct __access_info __file_perm_info[] = {
	{NFS4_ACE_READ_DATA, "read data", "r"},
	{NFS4_ACE_WRITE_DATA, "write data", "w"},
	{NFS4_ACE_APPEND_DATA, "append data", "a"},
	{NFS4_ACE_EXECUTE, "execute", "x"},
	{NFS4_ACE_DELETE, "delete", "d" },
	{NFS4_ACE_READ_ATTRIBUTES, "read attrs", "t"},
	{NFS4_ACE_WRITE_ATTRIBUTES, "write attrs", "T"},
	{NFS4_ACE_READ_NAMED_ATTRS, "read named attrs", "n"},
	{NFS4_ACE_WRITE_NAMED_ATTRS, "write named attrs", "N"},
	{NFS4_ACE_READ_ACL, "read ACL", "c"},
	{NFS4_ACE_WRITE_ACL, "write ACL", "C"},
	{NFS4_ACE_WRITE_OWNER, "write owner", "o"},
	{NFS4_ACE_SYNCHRONIZE, "synchronize", "y"},
};

static const struct __access_info __dir_perm_info[] = {
	//{NFS4_ACE_LIST_DIRECTORY, "list directory", "l"},
	{NFS4_ACE_LIST_DIRECTORY, "list directory", "r"},
	//{NFS4_ACE_ADD_FILE, "add file", "f"},
	{NFS4_ACE_ADD_FILE, "add file", "w"},
	//{NFS4_ACE_ADD_SUBDIRECTORY, "add subdirectory", "s"},
	{NFS4_ACE_ADD_SUBDIRECTORY, "add subdirectory", "a"},
	{NFS4_ACE_EXECUTE, "execute", "x"},
	{NFS4_ACE_DELETE_CHILD, "delete child", "D"},
	{NFS4_ACE_DELETE, "delete", "d" },
	{NFS4_ACE_READ_ATTRIBUTES, "read attrs", "t"},
	{NFS4_ACE_WRITE_ATTRIBUTES, "write attrs", "T"},
	{NFS4_ACE_READ_NAMED_ATTRS, "read named attrs", "n"},
	{NFS4_ACE_WRITE_NAMED_ATTRS, "write named attrs", "N"},
	{NFS4_ACE_READ_ACL, "read ACL", "c"},
	{NFS4_ACE_WRITE_ACL, "write ACL", "C"},
	{NFS4_ACE_WRITE_OWNER, "write owner", "o"},
	{NFS4_ACE_SYNCHRONIZE, "synchronize", "y"},
};

/*
 * generic read:  synchronize, read ACL, read attributes, read data
 * generic write: synchronize, read ACL, write ACL, write attributes, append data, write data
 * generic exec:  synchronize, read ACL, read attributes, execute
 */
static const struct __access_info __extra_perm_info[] = {
	{NFS4_ACE_GENERIC_READ, "\"read\"", ""},
	{NFS4_ACE_GENERIC_WRITE, "\"write\"", ""},
	{NFS4_ACE_GENERIC_EXECUTE, "\"exec\"", ""},
	{NFS4_ACE_MASK_ALL, "all", ""},
};
			
static const struct __access_info __ace_type_info[] = {
	{NFS4_ACE_ACCESS_ALLOWED_ACE_TYPE, "ALLOW", "A"},
	{NFS4_ACE_ACCESS_DENIED_ACE_TYPE, "DENY", "D"},
	{NFS4_ACE_SYSTEM_AUDIT_ACE_TYPE, "AUDIT", "U"},
	{NFS4_ACE_SYSTEM_ALARM_ACE_TYPE, "ALARM", "L"},
};

static const struct __access_info __ace_flag_info[] = {
	{NFS4_ACE_FILE_INHERIT_ACE, "file-inherit", "f"},
	{NFS4_ACE_DIRECTORY_INHERIT_ACE, "directory-inherit", "d"},
	{NFS4_ACE_NO_PROPAGATE_INHERIT_ACE, "no-propagate-inherit", "n"},
	{NFS4_ACE_INHERIT_ONLY_ACE, "inherit-only", "i"},
	{NFS4_ACE_SUCCESSFUL_ACCESS_ACE_FLAG, "successful access", "S"},
	{NFS4_ACE_FAILED_ACCESS_ACE_FLAG, "failed access", "F"},
	{NFS4_ACE_IDENTIFIER_GROUP, "identifier group", "g"},
};

#define textForType(i) __ace_type_info[i].text
#define maskForType(i) __ace_type_info[i].mask
#define textForFlag(i) __ace_flag_info[i].text
#define maskForFlag(i) __ace_flag_info[i].mask
#define textForExtraPerm(i) __extra_perm_info[i].text
#define maskForExtraPerm(i) __extra_perm_info[i].mask

#define FILE_PERM_COUNT 13
#define FILE_PERM_COUNT1 7
#define FILE_PERM_COUNT2 6
#define DIR_PERM_COUNT  14
#define DIR_PERM_COUNT1 7
#define DIR_PERM_COUNT2 7
#define COLUMN1_PERM_COUNT 7
#define EXTRA_PERM_COUNT 4
#define TYPE_COUNT 4
#define FLAG_COUNT 6
 /* i'm thinking we'll handle isItAGroup? with the checkbox near the principal field;
  * so, i'm just skipping that last flag entry..
  * #define FLAG_COUNT 7
  */


class NFS4_ACL_Editor : public QMainWindow
{
	Q_OBJECT

	public:
		NFS4_ACL_Editor(QWidget *parent, char *filearg);
		void keyPressEvent(QKeyEvent *e);
		void keyReleaseEvent(QKeyEvent *e);
		void show();

		QWidget *w, *cw;
		QLabel *l, *fl;
		QFrame *f;
		QListWidget *lw;
		QListWidgetItem *lwi;
		QTableWidget *tw;
		QTableWidgetItem *twi;
		QStackedWidget *sw;
		QLineEdit *le;
		QCheckBox *gcb, *cb;
		QComboBox *cmb, *tcmb;
		QGroupBox *dpgb, *fpgb, *pgb, *fgb;
		QPushButton *pbu, *pbd, *pb, *rb, *sb, *ub, *db;
		QMenu *fileMenu, *actionsMenu, *editMenu, *preferencesMenu;
		DirFileDialog *dfd;

		QPalette normal, light;
		QColor qc;
		QColor qqc;
		QVBoxLayout *vbl, *tbl, *fbl;
		QHBoxLayout *hbl;
		QGridLayout *gl, *dgl, *fgl, *sgl;
		QButtonGroup *pbg, *dpbg, *fpbg, *sbg, *dsbg, *fsbg, *fbg, *tbg;
		QAction *a;

		QStringList sl;
		QString uis, gis, eis;
		QString uils, gils, eils;
		QString filename, formattedfilename;

		struct nfs4_acl *nacl;
		struct nfs4_ace *ace, *selectedACE;
		NFS4_ACL acl;
		char buf[16];
		int selectedACEIndex, selectedLogical, selectedVisual, deleting;
		int is_directory;


	protected:
		void reformatFilename();
		void populateMenus();
		void populateTypeWho();
		void populatePerms();
		void populatePerms(QGroupBox *qgb, QButtonGroup *qbg, int is_dir);
		void populateFlags();
		void populateButtons();
		void initWithFile(QString file);
		void initRowWithACE(int row, struct nfs4_ace *ace);
		void initRowWithList(int row, QStringList *t);
		void swapRows(int from, int to);
		QStringList *listForRow(int row);
		QIcon iconForRow(int row);
		QIcon iconForACE(struct nfs4_ace *ace);
		QIcon iconForACE(struct nfs4_ace *ace, int highlight);
		QIcon iconForPrincipal(const char *name, int is_group);

		#define setRowIcon(r, i) tw->verticalHeaderItem((r))->setIcon(i)
		#define setRowType(r, t) tw->item((r), 0)->setText(t)
		#define setRowPrincipal(r, p) tw->item((r), 1)->setText(p)
		#define setRowPerms(r, p) tw->item((r), 2)->setText(p)
		#define setRowFlags(r, f) tw->item((r), 3)->setText(f)

		void updateType();
		void updatePerms();
		void updateFlags();
		void updateCheckBoxesWithMask(QButtonGroup *bg, unsigned int mask);
		void syncWho(struct nfs4_ace *ace, const char *who);

		void move(int by);

		void doHighlight(int willDo);
		void activationChange(QEvent *e);
		//void changeEvent(QEvent *e);

	protected slots:
		void typeModified(int index);
		void principalModified(const QString &);
		void permsModified(QAbstractButton *a);
		void shortcutsModified(QAbstractButton *a);
		void flagsModified(QAbstractButton *a);
		void aceSelected(int row, int column, int prevrow, int prevcolumn);
		void aceMoved(int logical, int oldind, int newind);
		void moveUp();
		void moveDown();
		void getACL();
		void reloadACL();
		void newACE();
		void deleteACE();
		void setACL();
		void quit();
};

#endif
