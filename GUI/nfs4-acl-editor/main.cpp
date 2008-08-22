#include "nfs4acleditor.h"

int main(int argc, char **argv)
{
	char *filename = argv[1];
	QApplication app(argc, argv);
	NFS4_ACL_Editor	*ed = new NFS4_ACL_Editor(0, filename);

	ed->setWindowTitle("NFSv4 ACL Editor");
	ed->show();
	return app.exec();
}
