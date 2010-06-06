/*******************************************************************
 * MPKG packaging system
 * Package builder
 * $Id: mainwindow.cpp,v 1.58 2007/12/11 05:38:29 i27249 Exp $
 * ***************************************************************/

#include "mainwindow.h"

Form *guiObject;


MpkgErrorReturn qtErrorHandler(ErrorDescription err, const string& details) {
	return guiObject->errorHandler(err, details);
}

MpkgErrorReturn Form::errorHandler(ErrorDescription err, const string& details) {
	QMessageBox box(this);
	QVector<QPushButton *> buttons;
	if (err.action.size()>1) {
		for (size_t i=0; i<err.action.size(); ++i) {
			buttons.push_back(box.addButton(err.action[i].text.c_str(), QMessageBox::AcceptRole));
		}
	}
	box.setWindowTitle(tr("Error"));
	box.setText(err.text.c_str());
	box.setInformativeText(details.c_str());
	box.exec();
	if (err.action.size()==1) return err.action[0].ret;
	for (int i=0; i<buttons.size(); ++i) {
		if (box.clickedButton()==buttons[i]) return err.action[i].ret;
	}
	return MPKG_RETURN_ABORT;
}



Form::Form(QWidget *parent, string arg)
{
	mpkgErrorHandler.registerErrorHandler(qtErrorHandler);
	guiObject = this;

	sourcesAnalyzed = false;
	db = NULL;

	pBuilder_isStartup = true;
	if (arg.empty()) dataType=DATATYPE_NEW;
	_arg = arg;
	modified=false;
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
	short_description.resize(2);
	description.resize(2);
	packageDir = new QDir;
	packageDir->setSorting(QDir::DirsFirst | QDir::IgnoreCase);
	currentPackageDir = new QDir;
	currentPackageDir->setSorting(QDir::DirsFirst | QDir::IgnoreCase);

	currentFilesystemDir = new QDir(QDir::currentPath());
	currentFilesystemDir->setSorting(QDir::DirsFirst | QDir::IgnoreCase);

	if (parent==0) ui.setupUi(this);
	else ui.setupUi(parent);
	ui.preremoveEnabledCheckBox->hide();
	ui.postremoveEnabledCheckBox->hide();
	ui.preinstallEnabledCheckBox->hide();
	ui.postinstallEnabledCheckBox->hide();

	templateDirPath = QDir::homePath()+"/.packagebuilder/configure_templates/";
	debugLabel = new QLabel;
	debugLabel->setText("Debug window");
	ui.MaintainerNameEdit->setText(mConfig.getValue("maintainer_name").c_str());
	ui.MaintainerMailEdit->setText(mConfig.getValue("maintainer_email").c_str());

	ui.openDirectoryButton->hide();	
	//ui.DepTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	//ui.DepTableWidget->horizontalHeader()->hide();
	ui.DepTableWidget->verticalHeader()->hide();
	connect(ui.saveAndQuitButton, SIGNAL(clicked()), this, SLOT(saveAndExit()));
	connect(ui.saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
	connect(ui.addDepFromFilesBtn, SIGNAL(clicked()), this, SLOT(addDepsFromFiles()));
	connect(ui.importDescriptionButton, SIGNAL(clicked()), this, SLOT(importMetaFromFile()));

	connect(ui.aboutButton, SIGNAL(clicked()), this, SLOT(showAbout()));
	connect(ui.browsePatchButton, SIGNAL(clicked()), this, SLOT(browsePatch()));
// Build options UI switches

	connect(ui.editWithGvimButton, SIGNAL(clicked()), this, SLOT(editBuildScriptWithGvim()));
	connect(ui.loadScriptFromFileButton, SIGNAL(clicked()), this, SLOT(loadBuildScriptFromFile()));
	connect(ui.envOptionsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchEnvField(int)));
	switchEnvField(ui.envOptionsCheckBox->checkState());
	connect(ui.configureCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchConfigureField(int)));
	switchConfigureField(ui.configureCheckBox->checkState());
	connect(ui.compilationCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchCompilationField(int)));
	switchCompilationField(ui.compilationCheckBox->checkState());
	connect(ui.installationCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchInstallField(int)));
	switchInstallField(ui.installationCheckBox->checkState());
	connect(ui.sourcesRootDirectoryAutodetectCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchSourceDirectoryField(int)));
	switchSourceDirectoryField(ui.sourcesRootDirectoryAutodetectCheckBox->checkState());
	connect(ui.buildingSystemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(switchBuildSystem(int)));
	switchBuildSystem(ui.buildingSystemComboBox->currentIndex());
	connect(ui.cpuArchCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchCpuArchField(int)));
	switchCpuArchField(ui.cpuArchCheckBox->checkState());
	connect(ui.cpuTuneCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchCpuTuneField(int)));
	switchCpuTuneField(ui.cpuTuneCheckBox->checkState());
	connect(ui.optimizationCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchOptimizationField(int)));
	switchOptimizationField(ui.optimizationCheckBox->checkState());
	connect(ui.customGccOptionsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchGccOptionsField(int)));
	//connect(ui.advancedSourcesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchAdvancedUrlMode()));
	//switchAdvancedUrlMode();
	switchGccOptionsField(ui.customGccOptionsCheckBox->checkState());
	connect(ui.runScriptCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchScript(int)));
	switchScript(ui.runScriptCheckBox->checkState());
	connect(ui.runPreBuildScriptCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchPreBuildScript(int)));
	switchPreBuildScript(ui.runPreBuildScriptCheckBox->checkState());

	//connect(ui.advancedExtractComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(switchExtractPath()));

	//switchExtractPath();


	loadTemplateList();

	connect(ui.patchAddButton, SIGNAL(clicked()), this, SLOT(addPatch()));
	connect(ui.keyAddButton, SIGNAL(clicked()), this, SLOT(addKey()));
	connect(ui.patchDeleteButton, SIGNAL(clicked()), this, SLOT(deletePatch()));
	connect(ui.keyDeleteButton, SIGNAL(clicked()), this, SLOT(deleteKey()));

	connect(ui.downloadAnalyzeButton, SIGNAL(clicked()), this, SLOT(analyzeSources()));

	focusIndex=0;
	//connect(ui.filelistWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(setNewPackageCurrentDirectory(QListWidgetItem *)));
	connect(ui.filelistWidget, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(setNewPackageCurrentDirectory(QListWidgetItem *)));
	connect(ui.filelistWidget_2, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(setNewFilesystemCurrentDirectory(QListWidgetItem *)));

	connect(ui.goHomeButton, SIGNAL(clicked()), this, SLOT(goPackageHome()));
	connect(ui.goHomeButton_2, SIGNAL(clicked()), this, SLOT(goFilesystemHome()));
	connect(ui.goUpButton, SIGNAL(clicked()), this, SLOT(goPackageUp()));
	connect(ui.goUpButton_2, SIGNAL(clicked()), this, SLOT(goFilesystemUp()));

	connect(ui.copyButton, SIGNAL(clicked()), this, SLOT(copyFiles()));
	connect(ui.moveButton, SIGNAL(clicked()), this, SLOT(moveFiles()));
	connect(ui.deleteButton, SIGNAL(clicked()), this, SLOT(removeFiles()));
	connect(ui.renameButton, SIGNAL(clicked()), this, SLOT(renameFile()));
	connect(ui.viewButton, SIGNAL(clicked()), this, SLOT(viewFile()));
	connect(ui.editButton, SIGNAL(clicked()), this, SLOT(editFile()));
	connect(ui.mkdirButton, SIGNAL(clicked()), this, SLOT(makeDirectory()));
	connect(ui.filelistWidget, SIGNAL(iGotFocus()), this, SLOT(setPanel1Focus()));
	connect(ui.filelistWidget_2, SIGNAL(iGotFocus()), this, SLOT(setPanel2Focus()));
	connect(ui.refreshButton, SIGNAL(clicked()), this, SLOT(reloadPackageDirListing()));
	connect(ui.refreshButton_2, SIGNAL(clicked()), this, SLOT(reloadFilesystemDirListing()));
	connect(ui.cmdLine, SIGNAL(execCmd()), this, SLOT(execShellCommand()));
	connect(ui.urlEdit, SIGNAL(editingFinished()), this, SLOT(analyzeName_auto()));
	connect(ui.nameDetectButton, SIGNAL(clicked()), this, SLOT(analyzeName_forced()));
	connect(ui.setConfigTemplateButton, SIGNAL(clicked()), this, SLOT(applyTemplate()));
	connect(ui.saveConfigTemplateButton, SIGNAL(clicked()), this, SLOT(saveTemplate()));
	//connect(ui.advancedUrlAddButton, SIGNAL(clicked()), this, SLOT(addAdvancedUrl()));
	//connect(ui.advancedUrlDeleteButton, SIGNAL(clicked()), this, SLOT(deleteAdvancedUrl()));

	connect(ui.importFromDatabaseButton, SIGNAL(clicked()), this, SLOT(importFromDatabase()));
	connect(ui.resentComboBox, SIGNAL(activated(int)), this, SLOT(openResent(int)));
	ui.cmdLine->addItem("");
	reloadFilesystemDirListing();
	ui.cmdLine->setEnabled(false);
	this->showMaximized();
	ui.filelistCurrentPath->setText(tr("Package should be saved before using this tool"));
	//ui.filelistCurrentPath_2->setText(currentFilesystemDir->canonicalPath());

	if (!lockDatabase()) {
		QMessageBox::critical(this, tr("Error locking database"), tr("Cannot lock database because it is locked by another process"), QMessageBox::Ok);
	}
	else {
		db = new mpkg;
		vector<string> knownTags;
		db->get_available_tags(&knownTags);
		sort(knownTags.begin(), knownTags.end());
		for (unsigned int i=0; i<knownTags.size(); ++i) {
			ui.tagBox->addItem(knownTags[i].c_str());
		}
		delete db;
		db=NULL;
		unlockDatabase();
	}
	loadFile(arg.c_str());
	addToResent(arg);
}
Form::~Form() {
	if (db!=NULL) {
		delete db;
		unlockDatabase();
	}
}
void Form::addToResent(string filename) {
	
	string resentFile = QDir::homePath().toStdString() + "/.packagebuilder/resent";
	vector<string> resents = ReadFileStrings(resentFile);
	if (!FileExists(resentFile)) {
		system("mkdir -p " + QDir::homePath().toStdString() + "/.packagebuilder");
	}
	bool found = false;
	ui.resentComboBox->clear();
	ui.resentComboBox->addItem(tr("Open recent..."));
	for (unsigned int i=0; i<resents.size(); ++i) {
		if (resents[i]==filename) found = true;
		ui.resentComboBox->addItem(resents[i].c_str());
	}
	if (!found && !filename.empty()) {
		resents.insert(resents.begin(), filename);
		if (resents.size()>10) resents.resize(10);
		WriteFileStrings(resentFile, resents);
	}

}
void Form::openResent(int index) {
	//if (index==1) return;
	loadFile(ui.resentComboBox->itemText(index));
}
void Form::execShellCommand()
{
	if (ui.cmdLine->currentText().isEmpty()) return;
	QDir *curr;
	switch(focusIndex) {
		case 1:
			curr = currentPackageDir;
			break;
		case 2:
			curr = currentFilesystemDir;
			break;
		default: return;
	}
	QString cmd = "(cd " + curr->absolutePath() + "; " + ui.cmdLine->currentText() + ")";
	system(cmd.toStdString());
	reloadFilesystemDirListing();
	reloadPackageDirListing();
	ui.cmdLine->insertItem(1,cmd);
}
void Form::setCurrentPathLabel()
{
	if (focusIndex==1) ui.currentPath->setText("["+currentPackageDir->absolutePath()+"]# ");
	else ui.currentPath->setText("["+currentFilesystemDir->absolutePath()+"]# ");
}

void Form::loadTemplateList()
{
	ui.configTemplateComboBox->clear();
	QDir templateDir(templateDirPath);
	printf("templateDir: %s\n", templateDirPath.toStdString().c_str());
	if (!templateDir.exists()) {
		// Let's load system-wide templates if they exist
		templateDir.setPath("/etc/skel/.packagebuilder/configure_templates/");
		if (!templateDir.exists()) return;
	}
	QFileInfoList templateList = templateDir.entryInfoList();
	for (int i=0; i<templateList.size(); i++) {
		if (templateList[i].fileName()!=".." && templateList[i].fileName()!="." && !templateList[i].isDir()) ui.configTemplateComboBox->addItem(templateList[i].fileName());
	}
}

void Form::applyTemplate()
{
	QString filename = ui.configTemplateComboBox->currentText();
	if (filename.isEmpty()) return;
	vector<string> data;
	if (!FileExists(templateDirPath.toStdString() + filename.toStdString())) {
		if (!FileExists("/etc/skel/.packagebuilder/configure_templates/" + filename.toStdString())) {
			QMessageBox::critical(this, tr("Error loading template"), tr("Template file doesn't exist"));
			return;
		}
		else data = ReadFileStrings("/etc/skel/.packagebuilder/configure_templates/" + filename.toStdString());
		
	}
	else data = ReadFileStrings(templateDirPath.toStdString() + filename.toStdString());
	if (data.empty()) {
		QMessageBox::critical(this, tr("Error loading template"), tr("Template file is empty"));
		return;
	}
	keyList.clear();
	keys key_tmp;
	for (unsigned int i=0; i<data.size(); i++) {
		if (data[i]=="\n") continue;
		key_tmp.name.clear();
		key_tmp.value.clear();
		if (data[i].find("=")!=std::string::npos) {
			key_tmp.name = data[i].substr(0, data[i].find_first_of("="));
			if (data[i].find_first_of("=") < data[i].length()-1) key_tmp.value = data[i].substr(data[i].find_first_of("=")+1);
		}
		else key_tmp.name = data[i];
		keyList.push_back(key_tmp);
	}
	displayKeys();
}

void Form::saveTemplate()
{
	QString filename;
	bool filename_accepted=false;
	while(!filename_accepted) {
		filename = QInputDialog::getText(this, tr("Save as new template"), tr("Enter a template name:"));
		if (filename.isEmpty()) return;
		if (filename.contains("/")) {
			QMessageBox::critical(this, tr("Error saving template"), tr("Template name should not contain / symbol"));
		}
		else filename_accepted=true;
	}

	system("mkdir -p " + templateDirPath.toStdString());
	if (FileExists(templateDirPath.toStdString() + filename.toStdString()) && QMessageBox::question(this, tr("Confirm overwrite"), tr("This template already exists. Overwrite?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)!=QMessageBox::Yes) {
		QMessageBox::information(this, tr("Save cancelled"), tr("Template saving was cancelled."), QMessageBox::Ok);
		return;
	}
	string data;
	for (unsigned int i=0; i<keyList.size(); i++) {
		data+=keyList[i].name;
		if (!keyList[i].value.empty()) data+="="+keyList[i].value;
		data+="\n";
	}
	if (WriteFile(templateDirPath.toStdString() + filename.toStdString(), data)!=0) QMessageBox::critical(this, tr("Error saving template"), tr("Unable to write template file"));
	loadTemplateList();
}

void Form::saveTemplateNew() {

}
void Form::setPanel1Focus()
{
	reloadPackageDirListing();
	printf("Got panel 1 focus\n");
	focusIndex = 1;
	setCurrentPathLabel();
}
void Form::setPanel2Focus()
{
	reloadFilesystemDirListing();
	printf("Got panel 2 focus\n");
	focusIndex = 2;
	setCurrentPathLabel();
}

/*void Form::switchAdvancedUrlMode()
{
	bool flag = ui.advancedSourcesCheckBox->isChecked();
	ui.advancedUrlLabel->setEnabled(flag);
	ui.advancedUrlEdit->setEnabled(flag);
	ui.advancedUrlAddButton->setEnabled(flag);
	ui.advancedUrlDeleteButton->setEnabled(flag);
	ui.advancedUrlTable->setEnabled(flag);
	ui.advancedExtractToLabel->setEnabled(flag);
	ui.advancedExtractComboBox->setEnabled(flag);
}
void Form::loadAdvancedUrlTable()
{
	ui.advancedUrlTable->clearContents();
	ui.advancedUrlTable->setRowCount(0);
	for (int i=advancedUrlMap.size()-1; i>=0; i--) {
		ui.advancedUrlTable->insertRow(0);
		ui.advancedUrlTable->setItem(0,0,new QTableWidgetItem(advancedUrlMap.getKeyName(i).c_str()));
		ui.advancedUrlTable->setItem(0,1,new QTableWidgetItem(advancedUrlMap.getValue(i).c_str()));
	}
}
void Form::addAdvancedUrl()
{
	if (ui.advancedUrlEdit->text().isEmpty()) return;
	advancedUrlMap.setValue(ui.advancedUrlEdit->text().toStdString(), ui.extractPathEdit->text().toStdString());
	ui.advancedUrlEdit->clear();
	loadAdvancedUrlTable();
}
void Form::switchExtractPath()
{
	if (ui.advancedExtractComboBox->currentIndex()==0) { // Main sources
		ui.extractPathEdit->setEnabled(false);
		ui.extractPathEdit->setText("[main]");
	}
	if (ui.advancedExtractComboBox->currentIndex()==1) { // Separate
		ui.extractPathEdit->setEnabled(false);
		ui.extractPathEdit->setText("[separate]");
	}
	if (ui.advancedExtractComboBox->currentIndex()==2) { // Custom
		ui.extractPathEdit->setEnabled(true);
		ui.extractPathEdit->clear();
	}
	if (ui.advancedExtractComboBox->currentIndex()==3) { // Don't extract
		ui.extractPathEdit->setEnabled(false);
		ui.extractPathEdit->clear();
	}
}
void Form::deleteAdvancedUrl()
{
	int i = ui.advancedUrlTable->currentRow();
	if (i<0) return;
       	advancedUrlMap.deleteKey(i);
	loadAdvancedUrlTable();
}*/
void Form::embedSources()
{
	if (!sourcePackage.isSourceEmbedded(ui.urlEdit->text().toStdString()))
	{
		sourcePackage.embedSource(ui.urlEdit->text().toStdString());
	}
	
}
void Form::embedPatches()
{
	for (unsigned int i=0; i<patchList.size(); i++)
	{
		sourcePackage.embedPatch(patchList[i]);
	}
}

void Form::analyzeSources()
{
	SourceFile sFile;
	string configHelp;
	sFile.setUrl(ui.urlEdit->text().toStdString());
	bool fileExist = false;
	sFile.download(&fileExist, true);
	if (fileExist && QMessageBox::question(this, tr("File exists"), tr("The sources seems to be already downloaded. Do you want to re-download it?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::Yes) sFile.download(&fileExist, true);
	sFile.analyze(&configHelp);
	ui.buildingSystemComboBox->setCurrentIndex(sFile.getBuildType());
	if (sFile.getBuildType()==0) {
		QTextBrowser *browser = new QTextBrowser;
		browser->setText(configHelp.c_str());
		browser->showMaximized();
	}
	switchBuildSystem(ui.buildingSystemComboBox->currentIndex());
	analyzeName(false);
	sourcesAnalyzed = true;
	
}
void Form::buildAndInstall() {
	saveFile(false, true, true);
}
void Form::saveAndBuild() {
	saveFile(false, true);
}
void Form::analyzeName_auto()
{
	analyzeName(false);
}
void Form::analyzeName_forced()
{
	analyzeName(true);
}
void Form::analyzeName(bool force)
{
	// Try to analyze package name and version
	string filename = getFilename(ui.urlEdit->text().toStdString());
	if (getExtension(filename)=="gz") {
		filename = filename.substr(0, filename.length()-strlen(".gz"));
	}
	if (getExtension(filename)=="bz2") {
		filename = filename.substr(0,filename.length()-strlen(".bz2"));
	}
	if (getExtension(filename)=="zip") {
		filename = filename.substr(0,filename.length()-strlen(".zip"));
	}
	if (getExtension(filename)=="rar") {
		filename = filename.substr(0,filename.length()-strlen(".rar"));
	}
	if (getExtension(filename)=="tar") {
		filename = filename.substr(0,filename.length()-strlen(".tar"));
	}
	if (getExtension(filename)=="tgz" || getExtension(filename)=="tlz" || getExtension(filename)=="tbz" || getExtension(filename)=="txz") {
		filename = filename.substr(0,filename.length()-strlen(".tgz"));
	}

	string name, version;
	printf("Filename: %s\n", filename.c_str());
	if (filename.find("-")!=std::string::npos) {
		name = filename.substr(0, filename.find_last_of("-"));
		printf("Name: %s\n", name.c_str());
		if (name.length()<filename.length()) version = filename.substr(filename.find_last_of("-")+1);
	}
	if (ui.NameEdit->text().isEmpty() || force) ui.NameEdit->setText(name.c_str());
	if (ui.VersionEdit->text().isEmpty() || force)  ui.VersionEdit->setText(version.c_str()); // Will always enter generated version
	if (ui.BuildEdit->text().isEmpty() || force) ui.BuildEdit->setText("1");

}
string cleanDescr(string str)
{
	string ret;
	if (str.find_first_of("\n")==std::string::npos) return str;
	while (str.find_first_of("\n")!=std::string::npos)
	{
		ret += str.substr(0,str.find_first_of("\n"));
		if (str.find_first_of("\n")<str.length()-1) str=str.substr(str.find_first_of("\n")+1);
		else {
			ret+=str;
			str.clear();
		}
	}
	ret+=str;
	return ret;
}
void Form::loadData()
{
	int ret;
	if (modified) {

		ret = (QMessageBox::warning(this, tr("MPKG package builder"),
                   tr("The document has been modified.\n"
                      "Do you want to save your changes?"),
                   QMessageBox::Save | QMessageBox::Discard
                   | QMessageBox::Cancel,
                   QMessageBox::Save));
		if (ret == QMessageBox::Save) saveFile();
	}

	loadFile("");
}


void Form::loadFile(QString filename)
{
	reloadFilesystemDirListing();
	reloadPackageDirListing();

	if (filename.isEmpty() && pBuilder_isStartup) {
		pBuilder_isStartup=false;
		return;
	}
	if (filename.isEmpty() && !pBuilder_isStartup)
	{
		QFileDialog fd;
		fd.setOption(QFileDialog::DontUseNativeDialog);
		filename = fd.getOpenFileName(this, tr("Open SPKG or package"), "", tr("SPKG(*.spkg);;Package (*.tgz *.txz *.tlz *.tbz)"));
		if (filename.isEmpty()) return;
	}

	dataType = DATATYPE_UNKNOWN;
	if (getExtension(filename.toStdString())=="tgz" || getExtension(filename.toStdString())=="txz" || getExtension(filename.toStdString())=="tlz" || getExtension(filename.toStdString())=="tbz") dataType = DATATYPE_BINARYPACKAGE;
	if (getExtension(filename.toStdString())=="spkg") dataType = DATATYPE_SOURCEPACKAGE;
	if (getExtension(filename.toStdString())=="xml") dataType = DATATYPE_XML;
	if (isDirectory(filename.toStdString())) dataType = DATATYPE_DIR;

	if (dataType == DATATYPE_UNKNOWN) {
		QMessageBox::critical(this, tr("Cannot open ") + filename, tr("Unable to open file/directory ") + filename + tr(", because it isn't a supported package type"));
		return;
	}
	if (dataType == DATATYPE_DIR)
	{
		if (QMessageBox::question(this, tr("Cannot determine tree type"), tr("Is it a binary package tree?"), 
					QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)==QMessageBox::Yes)
		{
			binaryPackage.importDirectory(filename.toStdString());
			dataType = DATATYPE_BINARYPACKAGE;
		}
		else {
			sourcePackage.importDirectory(filename.toStdString());
			dataType = DATATYPE_SOURCEPACKAGE;
		}
	}
	PackageConfig *p;
	switch(dataType)
	{
		case DATATYPE_BINARYPACKAGE:
			if (!binaryPackage.setInputFile(filename.toStdString())) {
				mError("Cannot open file"); // TODO: Replace to GUI error message
				return;
			}
			if (!binaryPackage.unpackFile()) {
				mError("Error while opening file: cannot extract");
				return;
			}
			//ui.directoryPath->setText(binaryPackage.pkg_dir.c_str());
			p = new PackageConfig(binaryPackage.getDataFilename());
			if (!p->parseOk) {
				mError("Error parsing XML data");
				return;
			}
			ui.sourcePackageRadioButton->setChecked(false);
			ui.binaryPackageRadioButton->setChecked(true);
			ui.packageTypeGroupBox->setEnabled(false);
			break;

		case DATATYPE_SOURCEPACKAGE:
			if (!sourcePackage.setInputFile(filename.toStdString())) {
				mError("Cannot open file");
				return;
			}
			if (!sourcePackage.unpackFile()) {
				mError("Error while opening file: cannot extract");
				return;
			}
			//ui.directoryPath->setText(sourcePackage.pkg_dir.c_str());

			p = new PackageConfig(sourcePackage.getDataFilename());
			if (!p->parseOk) {
				mError("Error parsing XML data");
				return;
			}
			ui.sourcePackageRadioButton->setChecked(true);
			ui.binaryPackageRadioButton->setChecked(false);
			ui.packageTypeGroupBox->setEnabled(false);

			break;
		case DATATYPE_XML:
			p = new PackageConfig(filename.toStdString());
			if (!p->parseOk) {
				mError("Error parsing XML data");
				return;
			}
			break;
		default:
			mError("Unknown data type");
			return;

	}
	// Storing in resent list
	addToResent(filename.toStdString());

	// Loading sources info
	ui.urlEdit->setText(p->getBuildUrl().c_str());
	if (dataType==DATATYPE_SOURCEPACKAGE) {
		if (sourcePackage.isSourceEmbedded(ui.urlEdit->text().toStdString())) ui.embedSourcesCheckBox->setCheckState(Qt::Checked);
		else ui.embedSourcesCheckBox->setCheckState(Qt::Unchecked);
	}
	//string advUrl;
	//advancedUrlMap = p->getBuildAdvancedUrlMap();
	//loadAdvancedUrlTable();
//	ui.advancedSourcesCheckBox->setChecked(!advancedUrlMap.empty());
//	switchAdvancedUrlMode();

	if (p->getBuildNoSubfolder()) ui.noSubfolderCheckBox->setCheckState(Qt::Checked);
	else ui.noSubfolderCheckBox->setCheckState(Qt::Unchecked);
	if (p->getBuildSourceRoot().empty()) {
		ui.sourcesRootDirectoryAutodetectCheckBox->setCheckState(Qt::Checked);
		ui.sourcesRootDirectoryEdit->setText("");
	}
	else {
		ui.sourcesRootDirectoryAutodetectCheckBox->setCheckState(Qt::Unchecked);
		ui.sourcesRootDirectoryEdit->setText(p->getBuildSourceRoot().c_str());
	}
	if (p->getValue((const char *) GET_PKG_MBUILD_NOSTRIP)=="true") ui.noSubfolderCheckBox->setCheckState(Qt::Checked);
	else ui.noSubfolderCheckBox->setCheckState(Qt::Unchecked);
	patchList = p->getBuildPatchList();
	displayPatches();
// Directory listing
	
	QString path;
	switch(dataType) {
		case DATATYPE_NEW: debugLabel->setText("New package, no path still");
				   return;
				   break;
		case DATATYPE_SOURCEPACKAGE:
				   path = QString::fromStdString(sourcePackage.pkg_dir);
				   debugLabel->setText("Source package, path: " + path);
				   break;
		case DATATYPE_BINARYPACKAGE:
				   path = QString::fromStdString(binaryPackage.pkg_dir);

				   debugLabel->setText("Binary package, path: " + path);
				   break;
		default:
				   debugLabel->setText("Unknown/no datatype");
	}
	ui.filelistWidget->clear();
	packageDir->cd(path);
	currentPackageDir->cd(path);
	reloadPackageDirListing();

	// stub for script path
	if (p->getBuildSystem()=="autotools") ui.buildingSystemComboBox->setCurrentIndex(0);
	if (p->getBuildSystem()=="scons") ui.buildingSystemComboBox->setCurrentIndex(1);
	if (p->getBuildSystem()=="cmake") ui.buildingSystemComboBox->setCurrentIndex(2);
	if (p->getBuildSystem()=="custom") ui.buildingSystemComboBox->setCurrentIndex(3);
	if (p->getBuildSystem()=="qmake4") ui.buildingSystemComboBox->setCurrentIndex(5);
	if (p->getBuildSystem()=="qmake3") ui.buildingSystemComboBox->setCurrentIndex(6);
	if (p->getBuildSystem()=="python") ui.buildingSystemComboBox->setCurrentIndex(7);
	if (p->getBuildSystem()=="make") ui.buildingSystemComboBox->setCurrentIndex(8);
	if (p->getBuildSystem()=="perl") ui.buildingSystemComboBox->setCurrentIndex(9);
	if (p->getBuildSystem()=="waf") ui.buildingSystemComboBox->setCurrentIndex(10);
	if (p->getBuildSystem()=="script") {
		ui.buildingSystemComboBox->setCurrentIndex(4);
	}
	if (!sourcePackage.readBuildScript().empty()) ui.runScriptCheckBox->setChecked(true);
	else ui.runScriptCheckBox->setChecked(false);
	switchScript(ui.runScriptCheckBox->checkState());
	ui.customScriptTextEdit->setText(sourcePackage.readBuildScript().c_str());

	if (!sourcePackage.readPrebuildScript().empty()) ui.runPreBuildScriptCheckBox->setChecked(true);
	else ui.runPreBuildScriptCheckBox->setChecked(false);
	switchPreBuildScript(ui.runPreBuildScriptCheckBox->checkState());
	ui.preBuildScriptTextEdit->setText(sourcePackage.readPrebuildScript().c_str());

	switchBuildSystem(ui.buildingSystemComboBox->currentIndex());

	if (p->getBuildConfigureEnvOptions().empty())
	{
		ui.envOptionsEdit->clear();
		ui.envOptionsCheckBox->setCheckState(Qt::Unchecked);
	}
	else
	{
		ui.envOptionsCheckBox->setCheckState(Qt::Checked);
		ui.envOptionsEdit->setText(p->getBuildConfigureEnvOptions().c_str());
	}


	if (p->getBuildOptimizationMarch().empty())
	{
		ui.cpuArchCheckBox->setCheckState(Qt::Unchecked);
	}
	else 
	{
		ui.cpuArchCheckBox->setCheckState(Qt::Checked);
		if (ui.cpuArchComboBox->findText(p->getBuildOptimizationMarch().c_str())<0) ui.cpuArchComboBox->addItem(p->getBuildOptimizationMarch().c_str());
		ui.cpuArchComboBox->setCurrentIndex(ui.cpuArchComboBox->findText(p->getBuildOptimizationMarch().c_str()));
	}
	if (p->getBuildOptimizationMtune().empty())
	{
		ui.cpuTuneCheckBox->setCheckState(Qt::Unchecked);
	}
	else 
	{
		ui.cpuTuneCheckBox->setCheckState(Qt::Checked);
		if (ui.cpuTuneComboBox->findText(p->getBuildOptimizationMtune().c_str())<0) ui.cpuTuneComboBox->addItem(p->getBuildOptimizationMtune().c_str());
		ui.cpuTuneComboBox->setCurrentIndex(ui.cpuTuneComboBox->findText(p->getBuildOptimizationMtune().c_str()));
	}
	if (p->getBuildOptimizationLevel().empty())
	{
		ui.optimizationCheckBox->setCheckState(Qt::Unchecked);
	}
	else 
	{
		ui.optimizationCheckBox->setCheckState(Qt::Checked);
		if (ui.optimizationComboBox->findText(p->getBuildOptimizationLevel().c_str())<0) ui.optimizationComboBox->addItem(p->getBuildOptimizationLevel().c_str());
		ui.optimizationComboBox->setCurrentIndex(ui.optimizationComboBox->findText(p->getBuildOptimizationLevel().c_str()));
	}
	if (p->getBuildOptimizationCustomGccOptions().empty()) 
	{
		ui.customGccOptionsCheckBox->setCheckState(Qt::Unchecked);
		ui.customGccOptionsEdit->setText("");
	}
	else {
		ui.customGccOptionsCheckBox->setCheckState(Qt::Checked);
		ui.customGccOptionsEdit->setText(p->getBuildOptimizationCustomGccOptions().c_str());
	}
		if (p->getBuildCmdConfigure().empty()) {
		ui.configureCheckBox->setCheckState(Qt::Unchecked);
		ui.configureEdit->setText("");
	}
	else {
		ui.configureCheckBox->setCheckState(Qt::Checked);
		ui.configureEdit->setText(p->getBuildCmdConfigure().c_str());
	}
	if (p->getBuildCmdMake().empty()) {
		ui.compilationCheckBox->setCheckState(Qt::Unchecked);
		ui.compilationEdit->setText("");
	}
	else {
		ui.compilationCheckBox->setCheckState(Qt::Checked);
		ui.compilationEdit->setText(p->getBuildCmdMake().c_str());
	}
	if (p->getBuildCmdMakeInstall().empty()) {
		ui.installationCheckBox->setCheckState(Qt::Unchecked);
		ui.installEdit->setText("");
	}
	else {
		ui.installationCheckBox->setCheckState(Qt::Checked);
		ui.installEdit->setText(p->getBuildCmdMakeInstall().c_str());
	}
	if (p->getBuildOptimizationCustomizable()) ui.AllowUserToChangeCheckBox->setCheckState(Qt::Checked);
	else ui.AllowUserToChangeCheckBox->setCheckState(Qt::Unchecked);

	if (p->getBuildUseCflags()) ui.dontUseCflags->setCheckState(Qt::Unchecked);
	else ui.dontUseCflags->setCheckState(Qt::Checked);

	ui.maxNumjobsSpinBox->setValue(atoi(p->getBuildMaxNumjobs().c_str()));
	keyList.clear();
	keys key_tmp;
	vector<string> key_names = p->getBuildKeyNames();
	vector<string> key_values = p->getBuildKeyValues();
	for (unsigned int i=0; i<key_names.size(); i++)
	{
		key_tmp.name=key_names[i];
		key_tmp.value=key_values[i];
		keyList.push_back(key_tmp);
	}
	displayKeys();

	// Loading scripts info
	switch(dataType) {
		case DATATYPE_BINARYPACKAGE:
			ui.preinstallScriptEdit->setText(binaryPackage.readPreinstallScript().c_str());
			ui.postinstallScriptEdit->setText(binaryPackage.readPostinstallScript().c_str());
			ui.preremoveScriptEdit->setText(binaryPackage.readPreremoveScript().c_str());
			ui.postremoveScriptEdit->setText(binaryPackage.readPostremoveScript().c_str());
			break;
		case DATATYPE_SOURCEPACKAGE:
			ui.preinstallScriptEdit->setText(sourcePackage.readPreinstallScript().c_str());
			ui.postinstallScriptEdit->setText(sourcePackage.readPostinstallScript().c_str());
			ui.preremoveScriptEdit->setText(sourcePackage.readPreremoveScript().c_str());
			ui.postremoveScriptEdit->setText(sourcePackage.readPostremoveScript().c_str());
			break;
		default:
			mWarning("This data type doesn't support scripts");
	}
	
	// Loading main package data
	PACKAGE pkg;
	xml2package(p->getXMLNode(), &pkg);

	// Filling data 
	ui.NameEdit->setText(pkg.get_name().c_str());
	ui.VersionEdit->setText(pkg.get_version().c_str());
	//ui.betaReleaseEdit->setText(pkg.get_betarelease().c_str());
	ui.ArchComboBox->setCurrentIndex(ui.ArchComboBox->findText(pkg.get_arch().c_str()));
	ui.BuildEdit->setText(pkg.get_build().c_str());
	ui.providesEdit->setText(pkg.get_provides().c_str());
	ui.conflictsEdit->setText(pkg.get_conflicts().c_str());
	ui.ShortDescriptionEdit->setText(pkg.get_short_description().c_str());
	short_description[0]=pkg.get_short_description().c_str();
	description[0]=cleanDescr(pkg.get_description()).c_str();
	ui.ShortDescriptionEdit->setText(short_description[0]);
	ui.DescriptionEdit->setText(description[0]);
	ui.ChangelogEdit->setText(pkg.get_changelog().c_str());
	ui.MaintainerNameEdit->setText(pkg.get_packager().c_str());
	ui.MaintainerMailEdit->setText(pkg.get_packager_email().c_str());
	ui.needSpecialUpdateCheckBox->setChecked(pkg.needSpecialUpdate);

	for (unsigned int i=0; i<pkg.get_dependencies().size(); i++)
	{
		ui.DepTableWidget->insertRow(0);
		ui.DepTableWidget->setItem(0,3, new QTableWidgetItem(pkg.get_dependencies().at(i).get_type().c_str()));
		ui.DepTableWidget->setItem(0,0, new QTableWidgetItem(pkg.get_dependencies().at(i).get_package_name().c_str()));
		ui.DepTableWidget->setItem(0,1, new QTableWidgetItem(pkg.get_dependencies().at(i).get_vcondition().c_str()));
		ui.DepTableWidget->setItem(0,2, new QTableWidgetItem(pkg.get_dependencies().at(i).get_package_version().c_str()));
		if (pkg.get_dependencies().at(i).isBuildOnly()) ui.DepTableWidget->setItem(0,4,new QTableWidgetItem("build_only"));
	}
	string tag_tmp;
	for (unsigned int i=0; i<pkg.get_tags().size(); i++)
	{
		tag_tmp=pkg.get_tags().at(i);
		ui.TagListWidget->addItem(tag_tmp.c_str());
		tag_tmp.clear();
	}
	
	reloadPackageDirListing();
	reloadFilesystemDirListing();
}

void Form::importMetaFromFile()
{
	QString file, xmlFile;
	string tmpfile;
	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);
       	file = fd.getOpenFileName(this, tr("Choose a file to import"));
	if (file.isEmpty()) return;
	if (getExtension(file.toStdString())=="xml") {
		// Pure XML
		xmlFile = file;
	}
	if (getExtension(file.toStdString())=="tgz" || getExtension(file.toStdString())=="txz" || getExtension(file.toStdString())=="tlz" || getExtension(file.toStdString())=="tbz" ) {
		// Binary package, need to extract XML first
		tmpfile = get_tmp_file();
		if (extractFromTgz(file.toStdString(), "install/data.xml", tmpfile)==0) xmlFile = tmpfile.c_str();
		else {
			unlink(tmpfile.c_str());
			return;
		}
	}
	if (getExtension(file.toStdString())=="spkg") {
		// Source package, need to extract XML first
		tmpfile = get_tmp_file();
		if (extractFromTgz(file.toStdString(), "install/data.xml", tmpfile)==0) xmlFile = tmpfile.c_str();
		else {
			unlink(tmpfile.c_str());
			return;
		}
	}
	// Process xmlFile
	if (!FileExists(xmlFile.toStdString())) {
		QMessageBox::warning(this, tr("Cannot import metadata"), tr("Cannot import metadata: XML file doesn't exist"));
		return;
	}
	
	PackageConfig p(xmlFile.toStdString());
	if (!p.parseOk) {
		QMessageBox::critical(this, tr("Error parsing metadata"), tr("Error while parsing XML, possible malformed structure"));
		return;
	}
	PACKAGE pkg;
	xml2package(p.getXMLNode(), &pkg);

	// Filling data
	ui.NameEdit->setText(pkg.get_name().c_str());
	ui.VersionEdit->setText(pkg.get_version().c_str());
	//ui.betaReleaseEdit->setText(pkg.get_betarelease().c_str());
	ui.ArchComboBox->setCurrentIndex(ui.ArchComboBox->findText(pkg.get_arch().c_str()));
	ui.BuildEdit->setText(pkg.get_build().c_str());
	ui.providesEdit->setText(pkg.get_provides().c_str());
	ui.conflictsEdit->setText(pkg.get_conflicts().c_str());

	ui.ShortDescriptionEdit->setText(pkg.get_short_description().c_str());
	short_description[0]=pkg.get_short_description().c_str();
	description[0]=cleanDescr(pkg.get_description()).c_str();
	ui.ShortDescriptionEdit->setText(short_description[0]);
	ui.DescriptionEdit->setText(description[0]);
	ui.ChangelogEdit->setText(pkg.get_changelog().c_str());
	ui.needSpecialUpdateCheckBox->setChecked(pkg.needSpecialUpdate);
	//ui.MaintainerNameEdit->setText(pkg.get_packager().c_str());
	//ui.MaintainerMailEdit->setText(pkg.get_packager_email().c_str());

	for (unsigned int i=0; i<pkg.get_dependencies().size(); i++) {
		ui.DepTableWidget->insertRow(0);
		ui.DepTableWidget->setItem(0,3, new QTableWidgetItem(pkg.get_dependencies().at(i).get_type().c_str()));
		ui.DepTableWidget->setItem(0,0, new QTableWidgetItem(pkg.get_dependencies().at(i).get_package_name().c_str()));
		ui.DepTableWidget->setItem(0,1, new QTableWidgetItem(pkg.get_dependencies().at(i).get_vcondition().c_str()));
		ui.DepTableWidget->setItem(0,2, new QTableWidgetItem(pkg.get_dependencies().at(i).get_package_version().c_str()));
		if (pkg.get_dependencies().at(i).isBuildOnly()) ui.DepTableWidget->setItem(0,4,new QTableWidgetItem("build_only"));
	}
	string tag_tmp;
	ui.TagListWidget->clear();
	for (unsigned int i=0; i<pkg.get_tags().size(); i++)
	{
		tag_tmp=pkg.get_tags().at(i);
		ui.TagListWidget->addItem(tag_tmp.c_str());
		tag_tmp.clear();
	}

}

bool Form::saveAs()
{
	return saveFile(true);
}
bool Form::saveFile(bool saveAsMode, bool buildAfterSave, bool installAfterBuild)
{
	// Check if all required fields are filled in
	if (ui.NameEdit->text().isEmpty() || ui.VersionEdit->text().isEmpty() || ui.BuildEdit->text().isEmpty()) {
		QMessageBox::warning(this, tr("Some required fields are empty"), tr("Please fill in all required fields (name, arch, version, build) first."));
		return false;
	}
	QString out_dir;
	if (dataType!=DATATYPE_NEW) {
		if (saveAsMode) {
			QFileDialog fd;
			fd.setOption(QFileDialog::DontUseNativeDialog);

			out_dir = fd.getExistingDirectory(this, tr("Where to save the package:"), "./");
			if (out_dir.isEmpty()) return false;
		}
		else {
			if (dataType==DATATYPE_SOURCEPACKAGE) {
				printf("input file: %s, abs: %s\n", sourcePackage.input_file.c_str(), getDirectory(getAbsolutePath(sourcePackage.input_file)).c_str());
				out_dir = QString::fromStdString(getDirectory(getAbsolutePath(sourcePackage.input_file)));
			}
			if (dataType==DATATYPE_BINARYPACKAGE) {
				out_dir = QString::fromStdString(getDirectory(getAbsolutePath(binaryPackage.input_file)));
			}
		}
	}

	if (dataType==DATATYPE_NEW)
	{
		QFileDialog fd;
		fd.setOption(QFileDialog::DontUseNativeDialog);

		out_dir = fd.getExistingDirectory(this, tr("Where to save the package:"), "./");
		if (out_dir.isEmpty()) return false;

		if (ui.sourcePackageRadioButton->isChecked()) {
			dataType = DATATYPE_SOURCEPACKAGE;
			sourcePackage.createNew();
		}

		if (ui.binaryPackageRadioButton->isChecked()) {
			dataType = DATATYPE_BINARYPACKAGE;
			binaryPackage.createNew();
		}
		if (dataType == DATATYPE_NEW)
		{
			QMessageBox::warning(this, "No package type specified", "Please specify package data first");
			return false;
		}
	}
	


	string xmlDir, xmlFilename;
	switch(dataType)
	{
		case DATATYPE_BINARYPACKAGE:
			xmlFilename = binaryPackage.getDataFilename();
			xmlDir = getDirectory(binaryPackage.getDataFilename());
			break;
		case DATATYPE_SOURCEPACKAGE:
			xmlFilename = sourcePackage.getDataFilename();
			xmlDir = getDirectory(sourcePackage.getDataFilename());
			break;
		default:
			mError("This type of file isn't supported");
			return false;
	}
	QString currentWindowTitle = _wTitle;
	setWindowTitle(_wTitle+tr(": saving, please wait..."));
	modified=false;

	string slack_desc;
	string slack_required;
	string slack_suggests;
	string desc_chunk;


	XMLNode node;
	node = XMLNode::createXMLTopNode("package");
	if (ui.needSpecialUpdateCheckBox->isChecked()) {
		node.addChild("need_special_update");
		node.getChildNode("need_special_update").addText("yes");
	}
	node.addChild("name");
	node.getChildNode("name").addText(ui.NameEdit->text().toStdString().c_str());
	node.addChild("version");
	node.getChildNode("version").addText(ui.VersionEdit->text().toStdString().c_str());
	/*if (!ui.betaReleaseEdit->text().isEmpty()) {
		node.addChild("betarelease");
		node.getChildNode("betarelease").addText(ui.betaReleaseEdit->text().toStdString().c_str());
	}*/
	node.addChild("arch");
	node.getChildNode("arch").addText(ui.ArchComboBox->currentText().toStdString().c_str());
	node.addChild("build");
	node.getChildNode("build").addText(ui.BuildEdit->text().toStdString().c_str());
	if (ui.providesEdit->text()!="0" && !ui.providesEdit->text().isEmpty()) {
		node.addChild("provides").addText(ui.providesEdit->text().toStdString().c_str());
	}
	
	if (ui.conflictsEdit->text()!="0" && !ui.conflictsEdit->text().isEmpty()) {
		node.addChild("conflicts").addText(ui.conflictsEdit->text().toStdString().c_str());
	}
	storeCurrentDescription();

	node.addChild("description");
	node.getChildNode("description",0).addAttribute("lang", "en");
	node.getChildNode("description",0).addText(description[0].toStdString().c_str());
	unsigned int spacePosition=0;
	slack_desc = ui.NameEdit->text().toStdString() + ": " + short_description[0].toStdString() + "\n" + ui.NameEdit->text().toStdString() + ": \n";
	for (unsigned int i=0; i<description[0].toStdString().length(); i++)
	{
		if (description[0].toStdString()[i]==' ') spacePosition = i;
		if (i >=70 || i == description[0].toStdString().length()-1)
		{
			desc_chunk = ui.NameEdit->text().toStdString() + ": " + description[0].toStdString().substr(0, spacePosition);
			if (i < description[0].toStdString().length()-1) i=0;
			if (spacePosition < description[0].toStdString().length()) description[0] = description[0].toStdString().substr(spacePosition).c_str();
			slack_desc+=desc_chunk + "\n";
		}
	}
	slack_desc = slack_desc.substr(0, slack_desc.length()-1) + description[0].toStdString();

	node.addChild("description");
	node.getChildNode("description",1).addAttribute("lang", "ru");
	node.getChildNode("description",1).addText(description[1].toStdString().c_str());

	node.addChild("short_description");
	node.getChildNode("short_description",0).addAttribute("lang", "en");
	node.getChildNode("short_description",0).addText(short_description[0].toStdString().c_str());
	node.addChild("short_description");
	node.getChildNode("short_description",1).addAttribute("lang", "ru");
	node.getChildNode("short_description",1).addText(short_description[1].toStdString().c_str());

	node.addChild("dependencies");

	node.addChild("suggests");
	int dcurr=0;
	int scurr=0;
	printf("Deps...\n");
	for (int i=0; i<ui.DepTableWidget->rowCount(); i++)
	{
		printf("[%d] Starting\n", i);
		if (ui.DepTableWidget->item(i,3)->text().toUpper()== "DEPENDENCY")
		{
			node.getChildNode("dependencies").addChild("dep");
			node.getChildNode("dependencies").getChildNode("dep", dcurr).addChild("name");
			node.getChildNode("dependencies").getChildNode("dep", dcurr).getChildNode("name").addText(ui.DepTableWidget->item(i,0)->text().toStdString().c_str());
			slack_required+=ui.DepTableWidget->item(i,0)->text().toStdString();
			node.getChildNode("dependencies").getChildNode("dep", dcurr).addChild("condition");
			node.getChildNode("dependencies").getChildNode("dep", dcurr).getChildNode("condition").addText(hcondition2xml(ui.DepTableWidget->item(i,1)->text().toStdString()).c_str());
			if (ui.DepTableWidget->item(i,1)->text().toStdString()!="any")
			{
				if (ui.DepTableWidget->item(i,1)->text().toStdString() == "==")
				{
					slack_required += "=";
				}
				else
				{
					slack_required+=ui.DepTableWidget->item(i,1)->text().toStdString();
				}
			}
			node.getChildNode("dependencies").getChildNode("dep", dcurr).addChild("version");
			node.getChildNode("dependencies").getChildNode("dep", dcurr).getChildNode("version").addText(ui.DepTableWidget->item(i,2)->text().toStdString().c_str());
			printf("[%d] Name, condition, version done\n", i);
			printf("SIZES: %dx%d\n", ui.DepTableWidget->rowCount(), ui.DepTableWidget->columnCount());
			if (false) ///!ui.DepTableWidget->item(i,4)->text().isEmpty()/* == "build_only"*/)
			{
				node.getChildNode("dependencies").getChildNode("dep",dcurr).addChild("build_only");
				node.getChildNode("dependencies").getChildNode("dep",dcurr).getChildNode("build_only").addText("true");
			}
			printf("[%d] build_only flag set\n",i);
			if (ui.DepTableWidget->item(i,1)->text().toStdString()!="any")
			{
				slack_required += ui.DepTableWidget->item(i,2)->text().toStdString() + "\n";
			}
			dcurr++;
		}
		if (ui.DepTableWidget->item(i,3)->text().toUpper()=="SUGGESTION" ||
				ui.DepTableWidget->item(i,3)->text().toUpper()=="SUGGEST")
		{
			node.getChildNode("suggests").addChild("suggest");
			node.getChildNode("suggests").getChildNode("suggest", scurr).addChild("name");
			node.getChildNode("suggests").getChildNode("suggest", scurr).getChildNode("name").addText(ui.DepTableWidget->item(i,0)->text().toStdString().c_str());
			node.getChildNode("suggests").getChildNode("suggest", scurr).addChild("condition");
			node.getChildNode("suggests").getChildNode("suggest", scurr).getChildNode("condition").addText(hcondition2xml(ui.DepTableWidget->item(i,1)->text().toStdString()).c_str());
			node.getChildNode("suggests").getChildNode("suggest", scurr).addChild("version");
			node.getChildNode("suggests").getChildNode("suggest", scurr).getChildNode("version").addText(ui.DepTableWidget->item(i,2)->text().toStdString().c_str());
			scurr++;
		}

	}
	node.addChild("tags");
	node.addChild("changelog");

	for (int i=0; i<ui.TagListWidget->count(); i++)
	{
		node.getChildNode("tags").addChild("tag");
		node.getChildNode("tags").getChildNode("tag",i).addText(ui.TagListWidget->item(i)->text().toStdString().c_str());
	}

	node.getChildNode("changelog").addText(ui.ChangelogEdit->toPlainText().toStdString().c_str());
	if (!ui.MaintainerNameEdit->text().isEmpty())
	{
		node.addChild("maintainer");
		node.getChildNode("maintainer").addChild("name");
		node.getChildNode("maintainer").getChildNode("name").addText(ui.MaintainerNameEdit->text().toStdString().c_str());
		if (!ui.MaintainerMailEdit->text().isEmpty())
		{
			node.getChildNode("maintainer").addChild("email");
			node.getChildNode("maintainer").getChildNode("email").addText(ui.MaintainerMailEdit->text().toStdString().c_str());
			if (mConfig.getValue("maintainer_email").empty()) mConfig.setValue("maintainer_email", ui.MaintainerMailEdit->text().toStdString());
		}
		if (mConfig.getValue("maintainer_name").empty()) mConfig.setValue("maintainer_name", ui.MaintainerNameEdit->text().toStdString());
	}
	node.addChild("configfiles");
	for (int i=0; i<ui.configFilesListWidget->count(); i++)
	{
		node.getChildNode("configfiles").addChild("conffile");
		node.getChildNode("configfiles").getChildNode("conffile",i).addText(ui.configFilesListWidget->item(i)->text().toStdString().c_str());
	}

	node.addChild("tempfiles");
	for (int i=0; i<ui.tempFilesListWidget->count(); i++)
	{
		node.getChildNode("tempfiles").addChild("tempfile");
		node.getChildNode("tempfiles").getChildNode("tempfile").addText(ui.tempFilesListWidget->item(i)->text().toStdString().c_str());
	}

	// Mbuild-related
	bool use_mbuild_data = !ui.urlEdit->text().isEmpty();
	if (use_mbuild_data) // Main URL should be always specified.
	{
		node.addChild("mbuild");
		if (!ui.urlEdit->text().isEmpty()) {
			node.getChildNode("mbuild").addChild("url");
			node.getChildNode("mbuild").getChildNode("url").addText(ui.urlEdit->text().toStdString().c_str());
		}
		/*if (ui.advancedSourcesCheckBox->isChecked() && !advancedUrlMap.empty()) {
			node.getChildNode("mbuild").addChild("source_list");
			for (unsigned int i=0; i<advancedUrlMap.size(); i++) {
				node.getChildNode("mbuild").getChildNode("source_list").addChild("source");
				node.getChildNode("mbuild").getChildNode("source_list").getChildNode("source", i).addText(advancedUrlMap.getKeyName(i).c_str());
				node.getChildNode("mbuild").getChildNode("source_list").getChildNode("source", i).addAttribute("extract_path", advancedUrlMap.getValue(i).c_str());
			}
		}*/
		if (!patchList.empty()) {
			node.getChildNode("mbuild").addChild("patches");
			for (unsigned int i=0; i<patchList.size(); i++)
			{
				node.getChildNode("mbuild").getChildNode("patches").addChild("patch");
				node.getChildNode("mbuild").getChildNode("patches").getChildNode("patch",i).addText(getFilename(patchList[i]).c_str());
			}
		}
		node.getChildNode("mbuild").addChild("sources_root_directory");
		if (ui.sourcesRootDirectoryAutodetectCheckBox->checkState()==Qt::Unchecked) 
			node.getChildNode("mbuild").getChildNode("sources_root_directory").addText(ui.sourcesRootDirectoryEdit->text().toStdString().c_str());
		node.getChildNode("mbuild").addChild("build_system");
		if (ui.noSubfolderCheckBox->isChecked()) {
			node.getChildNode("mbuild").addChild("no_subfolder");
			node.getChildNode("mbuild").getChildNode("no_subfolder").addText("true");
		}
		switch(ui.buildingSystemComboBox->currentIndex())
		{
			case 0: node.getChildNode("mbuild").getChildNode("build_system").addText("autotools");
				break;
			case 1: node.getChildNode("mbuild").getChildNode("build_system").addText("scons");
				break;
			case 2: node.getChildNode("mbuild").getChildNode("build_system").addText("cmake");
				break;
			case 3: node.getChildNode("mbuild").getChildNode("build_system").addText("custom");
				break;
			case 4: node.getChildNode("mbuild").getChildNode("build_system").addText("script");
				break;
			case 5: node.getChildNode("mbuild").getChildNode("build_system").addText("qmake4");
				break;
			case 6: node.getChildNode("mbuild").getChildNode("build_system").addText("qmake3");
				break;
			case 7: node.getChildNode("mbuild").getChildNode("build_system").addText("python");
				break;
			case 8: node.getChildNode("mbuild").getChildNode("build_system").addText("make");
				break;
			case 9: node.getChildNode("mbuild").getChildNode("build_system").addText("perl");
				break;
			case 10: node.getChildNode("mbuild").getChildNode("build_system").addText("waf");
				break;

		}

		node.getChildNode("mbuild").addChild("max_numjobs");
		node.getChildNode("mbuild").getChildNode("max_numjobs").addText(ui.maxNumjobsSpinBox->cleanText().toStdString().c_str());
		
		node.getChildNode("mbuild").addChild("optimization");
		if (ui.cpuArchCheckBox->checkState()==Qt::Checked) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("march");
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("march").addText(ui.cpuArchComboBox->currentText().toStdString().c_str());
		}
		if (ui.cpuTuneCheckBox->checkState()==Qt::Checked) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("mtune");
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("mtune").addText(ui.cpuTuneComboBox->currentText().toStdString().c_str());
		}
		if (ui.optimizationCheckBox->checkState()==Qt::Checked) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("olevel");
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("olevel").addText(ui.optimizationComboBox->currentText().toStdString().c_str());
		}
		if (ui.customGccOptionsCheckBox->checkState()==Qt::Checked) {
			node.getChildNode("mbuild").getChildNode("optimization").addChild("custom_gcc_options");
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("custom_gcc_options").addText(ui.customGccOptionsEdit->text().toStdString().c_str());
		}

		node.getChildNode("mbuild").getChildNode("optimization").addChild("allow_change");
		if (ui.AllowUserToChangeCheckBox->checkState()==Qt::Checked)
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("allow_change").addText("true");
		else   	
			node.getChildNode("mbuild").getChildNode("optimization").getChildNode("allow_change").addText("false");
	
		node.getChildNode("mbuild").addChild("use_cflags");	
		if (ui.dontUseCflags->isChecked())
			node.getChildNode("mbuild").getChildNode("use_cflags").addText("false");
		else
			node.getChildNode("mbuild").getChildNode("use_cflags").addText("true");

		if (ui.envOptionsCheckBox->checkState()==Qt::Checked)
		{
			node.getChildNode("mbuild").addChild("env_options");
			node.getChildNode("mbuild").getChildNode("env_options").addText(ui.envOptionsEdit->text().toStdString().c_str());
		}
		node.getChildNode("mbuild").addChild("configuration");
		for (unsigned int i=0; i<keyList.size(); i++)
		{
			node.getChildNode("mbuild").getChildNode("configuration").addChild("key");
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).addChild("name");
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).getChildNode("name").addText(keyList[i].name.c_str());
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).addChild("value");
			node.getChildNode("mbuild").getChildNode("configuration").getChildNode("key",i).getChildNode("value").addText(keyList[i].value.c_str());
		}
		if (ui.buildingSystemComboBox->currentIndex()==3)
		{
			node.getChildNode("mbuild").addChild("custom_commands");
			if (ui.configureCheckBox->checkState()==Qt::Checked)
			{
				node.getChildNode("mbuild").getChildNode("custom_commands").addChild("configure");
				node.getChildNode("mbuild").getChildNode("custom_commands").getChildNode("configure").addText(ui.configureEdit->text().toStdString().c_str());
			}
			if (ui.compilationCheckBox->checkState()==Qt::Checked)
			{
				node.getChildNode("mbuild").getChildNode("custom_commands").addChild("make");
				node.getChildNode("mbuild").getChildNode("custom_commands").getChildNode("make").addText(ui.compilationEdit->text().toStdString().c_str());
			}
			if (ui.installationCheckBox->checkState()==Qt::Checked)
			{
				node.getChildNode("mbuild").getChildNode("custom_commands").addChild("make_install");
				node.getChildNode("mbuild").getChildNode("custom_commands").getChildNode("make_install").addText(ui.installEdit->text().toStdString().c_str());
			}
		}
		if (ui.noStripCheckBox->isChecked()) {
			node.getChildNode("mbuild").addChild("nostrip").addText("true");
		}
	}

	node.writeToFile(xmlFilename.c_str());
	// Saving installation scripts
	if (WriteFile(xmlDir+"/slack-desc", slack_desc)!=0) {
		QMessageBox::critical(this, "Error saving package", "Error while saving package");
		return false;
	}

	if (!slack_required.empty()) WriteFile(xmlDir+"/slack-required", slack_required);
	string tmpF, tmpO;
	string pkgZType = mConfig.getValue("build_pkg_type");
	if (pkgZType.empty()) pkgZType = "tgz";
	switch(dataType)
	{
		case DATATYPE_BINARYPACKAGE:
			binaryPackage.setPostinstallScript(ui.postinstallScriptEdit->toPlainText().toStdString());
			binaryPackage.setPostremoveScript(ui.postremoveScriptEdit->toPlainText().toStdString());
			binaryPackage.setPreinstallScript(ui.preinstallScriptEdit->toPlainText().toStdString());
			binaryPackage.setPreremoveScript(ui.preremoveScriptEdit->toPlainText().toStdString());
			tmpF = ui.NameEdit->text().toStdString()+"-"+ui.VersionEdit->text().toStdString()+"-" + ui.ArchComboBox->currentText().toStdString() +"-"+ui.BuildEdit->text().toStdString() + "." + pkgZType;
			binaryPackage.packFile(out_dir.toStdString(), &tmpO);
			addToResent(tmpO + "/" + tmpF);
			break;
		case DATATYPE_SOURCEPACKAGE:
			sourcePackage.setPostinstallScript(ui.postinstallScriptEdit->toPlainText().toStdString());
			sourcePackage.setPostremoveScript(ui.postremoveScriptEdit->toPlainText().toStdString());
			sourcePackage.setPreinstallScript(ui.preinstallScriptEdit->toPlainText().toStdString());
			sourcePackage.setPreremoveScript(ui.preremoveScriptEdit->toPlainText().toStdString());


			if (ui.embedSourcesCheckBox->isChecked()) embedSources();
			else sourcePackage.removeSource();
			if (patchList.empty()) sourcePackage.removeAllPatches();
			embedPatches();
			if (ui.buildingSystemComboBox->currentIndex()==4 || ui.runScriptCheckBox->isChecked()) sourcePackage.setBuildScript(ui.customScriptTextEdit->toPlainText().toStdString());
			else sourcePackage.setBuildScript("");
			if (ui.buildingSystemComboBox->currentIndex()==4 || ui.runPreBuildScriptCheckBox->isChecked()) sourcePackage.setPrebuildScript(ui.preBuildScriptTextEdit->toPlainText().toStdString());
			else sourcePackage.setPrebuildScript("");
			tmpF=ui.NameEdit->text().toStdString() + "-" + ui.VersionEdit->text().toStdString()+ "-" + ui.BuildEdit->text().toStdString()+".spkg";
			printf("out_dir = %s, filename = %s\n", out_dir.toStdString().c_str(), tmpF.c_str());
			sourcePackage.packFile(out_dir.toStdString(), &tmpO);
			addToResent(tmpO +"/" + tmpF);
			if (buildAfterSave) {
				printf("Calling build %s / %s\n", tmpO.c_str(), tmpF.c_str());
				runBuild(tmpO + "/" + tmpF, installAfterBuild);
			}
			break;
	}

	setWindowTitle(_wTitle+tr(" (saved)"));
	modified=false;

	reloadPackageDirListing();
	reloadFilesystemDirListing();
	return true;
}

void Form::runBuild(string filename, bool installAfterBuild) {
	if (dataType!=DATATYPE_SOURCEPACKAGE) return;
	string buildCmd = "mpkg build";
	if (installAfterBuild) buildCmd = "mpkg-install -y";
	system("xterm -e 'echo Building " + filename + " && ( " + buildCmd + " " + filename + " || read ) && echo Press any key to close window && read' &");
}
void Form::showAbout()
{
	QMessageBox::information(this, tr("About packagebuilder"), tr("Package metadata builder for MPKG ") + (QString) mpkgVersion + " (build " + (QString) mpkgBuild + tr(")\n\n(c) RPU NET (www.rpunet.ru)\nLicensed under GPL"), QMessageBox::Ok, QMessageBox::Ok);
}
void Form::loadBuildScriptFromFile()
{
	QFileDialog fd;
	fd.setOption(QFileDialog::DontUseNativeDialog);

	QString importFileName = fd.getOpenFileName(this, tr("Select a script file"));
	if (importFileName.isEmpty()) return;
	if (!FileExists(importFileName.toStdString())) {
		QMessageBox::warning(this, "Error importing script", "The specified file doesn't exist");
		return;
	}
	ui.customScriptTextEdit->setPlainText(ReadFile(importFileName.toStdString()).c_str());
}
	

void Form::editBuildScriptWithGvim()
{
	if (tempScriptEditFile.empty())
	{
		tempScriptEditFile = get_tmp_file();
		WriteFile(tempScriptEditFile, ui.customScriptTextEdit->toPlainText().toStdString());
		system("gvim " + tempScriptEditFile);
		ui.editWithGvimButton->setText(tr("Finish editing with gvim"));
	}
	else {
		ui.customScriptTextEdit->setPlainText(ReadFile(tempScriptEditFile).c_str());
		unlink(tempScriptEditFile.c_str());
		tempScriptEditFile.clear();
		ui.editWithGvimButton->setText(tr("Edit with gvim"));
	}
}

void Form::addTag(){
	if (!ui.tagBox->currentText().isEmpty())
	{
		ui.TagListWidget->addItem(ui.tagBox->currentText());
	}
}

void Form::saveAndExit()
{
	if (saveFile()) quitApp();
}
void Form::addDepsFromFiles()
{
	QStringList files;
	QFileDialog *dialog = new QFileDialog;
	dialog->setFileMode(QFileDialog::ExistingFiles);
	dialog->setOption(QFileDialog::DontUseNativeDialog);
	if (dialog->exec()) files = dialog->selectedFiles();
	PackageConfig *p;
	string tmp_xml = get_tmp_file();
	int e_res=-1;
	for (int i=0; i<files.size(); i++)
	{
		if (getExtension(files.at(i).toStdString())=="tgz" || getExtension(files.at(i).toStdString())=="txz" || getExtension(files.at(i).toStdString())=="tlz" || getExtension(files.at(i).toStdString())=="tbz" )
			e_res = extractFromTgz(files.at(i).toStdString(), "install/data.xml", tmp_xml);
		if (getExtension(files.at(i).toStdString())=="spkg")
			e_res = extractFromTar(files.at(i).toStdString(), "install/data.xml", tmp_xml);
		if (e_res==0)
		{
			p = new PackageConfig(tmp_xml);
			ui.DepNameEdit->setText(p->getName().c_str());
			ui.DepConditionComboBox->setCurrentIndex(1);
			ui.DepVersionEdit->setText(p->getVersion().c_str());
			addDependency();
			delete p;
			unlink(tmp_xml.c_str());
		}
		else QMessageBox::warning(this, tr("Error importing dependencies"), tr("Cannot import dependency from file ") + files.at(i), QMessageBox::Ok, QMessageBox::Ok);
	}
}

void Form::addDependency(){

	if (!ui.DepNameEdit->text().isEmpty())
	{
		if (ui.DepConditionComboBox->currentText()=="any" || !ui.DepVersionEdit->text().isEmpty())
		{
			ui.DepTableWidget->insertRow(0);
			ui.DepTableWidget->setItem(0,3, new QTableWidgetItem(ui.DepSuggestComboBox->currentText()));
			ui.DepTableWidget->setItem(0,0, new QTableWidgetItem(ui.DepNameEdit->text()));
			ui.DepTableWidget->setItem(0,1, new QTableWidgetItem(ui.DepConditionComboBox->currentText()));
			ui.DepTableWidget->setItem(0,2, new QTableWidgetItem(ui.DepVersionEdit->text()));
			if (ui.buildDependencyCheckBox->isChecked()) ui.DepTableWidget->setItem(0,4, new QTableWidgetItem("build_only"));
			else ui.DepTableWidget->setItem(0,4,new QTableWidgetItem(""));
			ui.DepNameEdit->clear();
			ui.DepVersionEdit->clear();
			ui.DepSuggestComboBox->setCurrentIndex(0);
			ui.DepConditionComboBox->setCurrentIndex(0);
		}
	}
}
void Form::addConfigFile()
{
	if (!ui.confFileAddEdit->text().isEmpty())
	{
		QListWidgetItem *__item = new QListWidgetItem(ui.configFilesListWidget);
		__item->setText(ui.confFileAddEdit->text());
	}
}

void Form::deleteConfigFile()
{
	int i=ui.configFilesListWidget->currentRow();
	ui.configFilesListWidget->takeItem(i);
}
void Form::deleteTag()
{
	int i=ui.TagListWidget->currentRow();
	ui.TagListWidget->takeItem(i);
}
void Form::searchConfigFile()
{
	QString pkgRoot;
	if (dataType==DATATYPE_BINARYPACKAGE) pkgRoot = binaryPackage.getExtractedPath().c_str();

	if (pkgRoot.isEmpty())
	{
		QFileDialog fd;
		fd.setOption(QFileDialog::DontUseNativeDialog);

		pkgRoot = fd.getExistingDirectory(this, tr("Choose package root"), "./");
	}
	if (!pkgRoot.isEmpty()) 
	{
		QFileDialog fd;
		fd.setOption(QFileDialog::DontUseNativeDialog);

		QString fname = fd.getOpenFileName(this, tr("Choose a config file"), pkgRoot, "");
		if (fname.length()>pkgRoot.length())
		ui.confFileAddEdit->setText(fname.toStdString().substr(pkgRoot.length()).c_str());
	}
}
void Form::searchTempFile()
{
	QString pkgRoot;
	if (dataType==DATATYPE_BINARYPACKAGE) pkgRoot = binaryPackage.getExtractedPath().c_str();

	if (pkgRoot.isEmpty())
	{
		QFileDialog fd;
		fd.setOption(QFileDialog::DontUseNativeDialog);
		pkgRoot = fd.getExistingDirectory(this, tr("Choose package root"), "./");
	}
	if (!pkgRoot.isEmpty()) 
	{
		QFileDialog fd;
		fd.setOption(QFileDialog::DontUseNativeDialog);
		QString fname = fd.getOpenFileName(this, tr("Choose a config file"), pkgRoot, "");
		if (fname.length()>pkgRoot.length())
		ui.tempFileAddEdit->setText(fname.toStdString().substr(pkgRoot.length()).c_str());
	}
}

void Form::deleteDependency()
{
	int i=ui.DepTableWidget->currentRow();
	ui.DepTableWidget->removeRow(i);
}
void Form::changeHeader()
{
	modified=true;

	QString FLabel=tr("MPKG package builder");

	if (!ui.NameEdit->text().isEmpty())
	{
		FLabel+=": "+ui.NameEdit->text();
		if (!ui.VersionEdit->text().isEmpty())
		{
			FLabel+="-"+ui.VersionEdit->text()+"-"+ui.ArchComboBox->currentText();
			if (!ui.BuildEdit->text().isEmpty())
			{
				FLabel+="-"+ui.BuildEdit->text();
			}
		}
	}
	setWindowTitle(FLabel);
	_wTitle = FLabel;
}

void Form::changeHeader(const QString &)
{
	/*if (!text.isEmpty())
	{
		setWindowTitle(text);
		return;
	}*/
	modified=true;
	QString FLabel=tr("MPKG package builder");

	if (!ui.NameEdit->text().isEmpty())
	{
		FLabel+=": "+ui.NameEdit->text();
		if (!ui.VersionEdit->text().isEmpty())
		{
			FLabel+="-"+ui.VersionEdit->text()+"-"+ui.ArchComboBox->currentText();
			if (!ui.BuildEdit->text().isEmpty())
			{
				FLabel+="-"+ui.BuildEdit->text();
			}
		}
	}
	setWindowTitle(FLabel);
	_wTitle = FLabel;
}

void Form::swapLanguage()
{
	
	int i;
	int i2;
	/*if (ui.DescriptionLanguageComboBox->currentText()=="ru")
	{
		i=0;
		i2=1;
	}
	else 
	{*/
		i=1;
		i2=0;
	//}

	short_description[i]=ui.ShortDescriptionEdit->text();
	description[i]=ui.DescriptionEdit->toPlainText();
	ui.ShortDescriptionEdit->setText(short_description[i2]);
	ui.DescriptionEdit->setPlainText(description[i2]);
}

void Form::storeCurrentDescription()
{
	int i;
	/*if (ui.DescriptionLanguageComboBox->currentText()=="ru")
	{
		i=1;
	}
	else 
	{*/
		i=0;
	//}

	short_description[i]=ui.ShortDescriptionEdit->text();
	description[i]=ui.DescriptionEdit->toPlainText();

}

void Form::quitApp()
{
	int ret;
	if (modified)
	{
		ret = QMessageBox::warning(this, tr("MPKG package builder"),
                   tr("The document has been modified.\n"
                      "Do you want to save your changes?"),
                   QMessageBox::Save | QMessageBox::Discard
                   | QMessageBox::Cancel,
                   QMessageBox::Save);
		switch(ret)
		{
			case QMessageBox::Save: 
				saveFile();
				qApp->quit();
				break;
			case QMessageBox::Discard:
				qApp->quit();
				break;
			case QMessageBox::Cancel:
				break;
		}
	}
	else qApp->quit();
}

void Form::switchEnvField(int state)
{
	if (state==Qt::Checked) ui.envOptionsEdit->setEnabled(true);
	else ui.envOptionsEdit->setEnabled(false);
}
void Form::switchConfigureField(int state)
{
	if (state==Qt::Checked) ui.configureEdit->setEnabled(true);
	else ui.configureEdit->setEnabled(false);
}
void Form::switchCompilationField(int state)
{
	if (state==Qt::Checked) ui.compilationEdit->setEnabled(true);
	else ui.compilationEdit->setEnabled(false);

}
void Form::switchInstallField(int state)
{
	if (state==Qt::Checked) ui.installEdit->setEnabled(true);
	else ui.installEdit->setEnabled(false);

}
void Form::switchSourceDirectoryField(int state)
{
	if (state==Qt::Checked) ui.sourcesRootDirectoryEdit->setEnabled(false);
	else ui.sourcesRootDirectoryEdit->setEnabled(true);

}
void Form::switchBuildSystem(int index)
{
	if (index<3) {
		//ui.runScriptCheckBox->setVisible(true);
		//ui.optimizationGroupBox->setVisible(true);
		//if (!ui.runScriptCheckBox->isChecked()) ui.customScriptGroupBox->setVisible(false);
		//else ui.customScriptGroupBox->setVisible(true);
		ui.customCommandsGroupBox->setVisible(false);
		//ui.compilationGroupBox->setVisible(true);

	}
	if (index==4)
	{
		//ui.runScriptCheckBox->setVisible(false);
		//ui.optimizationGroupBox->setVisible(true);
		//ui.customScriptGroupBox->setVisible(true);
		ui.customCommandsGroupBox->setVisible(false);
		//ui.compilationGroupBox->setVisible(false);

	}
	if (index==3)
	{
		//ui.runScriptCheckBox->setVisible(true);
		//ui.optimizationGroupBox->setVisible(true);
		//if (ui.runScriptCheckBox->isChecked()) ui.customScriptGroupBox->setVisible(false);
		//else ui.customScriptGroupBox->setVisible(true);
		ui.customCommandsGroupBox->setVisible(true);
		//ui.compilationGroupBox->setVisible(true);
	}


}
void Form::switchCpuArchField(int state)
{
	if (state==Qt::Checked) ui.cpuArchComboBox->setEnabled(true);
	else ui.cpuArchComboBox->setEnabled(false);

}
void Form::switchCpuTuneField(int state)
{
	if (state==Qt::Checked) ui.cpuTuneComboBox->setEnabled(true);
	else ui.cpuTuneComboBox->setEnabled(false);

}
void Form::switchOptimizationField(int state)
{
	if (state==Qt::Checked) ui.optimizationComboBox->setEnabled(true);
	else ui.optimizationComboBox->setEnabled(false);

}
void Form::switchGccOptionsField(int state)
{
	if (state==Qt::Checked) ui.customGccOptionsEdit->setEnabled(true);
	else ui.customGccOptionsEdit->setEnabled(false);

}
void Form::switchScript(int state)
{
	if (state==Qt::Checked) ui.customScriptTextEdit->setEnabled(true);
	else ui.customScriptTextEdit->setEnabled(false);
}

void Form::switchPreBuildScript(int state)
{
	ui.preBuildScriptTextEdit->setEnabled(state==Qt::Checked);
}

void Form::displayKeys()
{
	ui.compilationOptionsTableWidget->clear();
	ui.compilationOptionsTableWidget->setColumnCount(2);
	ui.compilationOptionsTableWidget->setRowCount(keyList.size());
	for (unsigned int i=0; i<keyList.size(); i++)
	{
		ui.compilationOptionsTableWidget->setItem(i,0, new QTableWidgetItem(keyList[i].name.c_str()));
		ui.compilationOptionsTableWidget->setItem(i,1, new QTableWidgetItem(keyList[i].value.c_str()));
	}

}

void Form::displayPatches()
{
	ui.patchListWidget->clear();
	for (unsigned int i=0; i<patchList.size(); i++)
	{
		QListWidgetItem *__item = new QListWidgetItem(ui.patchListWidget);
		__item->setText(patchList[i].c_str());
	}
}
void Form::browsePatch()
{
	QStringList files;
	QFileDialog *dialog = new QFileDialog;
	dialog->setFileMode(QFileDialog::ExistingFiles);
	dialog->setOption(QFileDialog::DontUseNativeDialog);
	if (dialog->exec()) files = dialog->selectedFiles();
	for (int i=0; i<files.size(); i++) {
		ui.patchEdit->setText(files[i]);
		addPatch();
	}
}

void Form::addPatch()
{
	if (!ui.patchEdit->text().isEmpty()) {
		patchList.push_back(ui.patchEdit->text().toStdString());
		ui.patchEdit->setText("");
		displayPatches();
	}
}

void Form::addKey()
{
	if (!ui.keyNameEdit->text().isEmpty())
	{
		keys key_tmp;
		key_tmp.name = ui.keyNameEdit->text().toStdString();
		key_tmp.value = ui.keyValueEdit->text().toStdString();
		keyList.push_back(key_tmp);
		displayKeys();
		ui.keyNameEdit->clear();
		ui.keyValueEdit->clear();
	}
}

void Form::deletePatch()
{
	int i = ui.patchListWidget->currentRow();
	if (i>=0 && (unsigned int) i<patchList.size())
	{
		vector<string> copy;
		for (unsigned int t=0; t<patchList.size(); t++)
		{
			if (t!=(unsigned int) i) copy.push_back(patchList[t]);
		}
		patchList=copy;
		displayPatches();
	}
}

void Form::deleteKey()
{
	int i = ui.compilationOptionsTableWidget->currentRow();
	if (i>=0 && (unsigned int) i<keyList.size())
	{
		vector<keys> copy;
		for (unsigned int t=0; t<keyList.size(); t++)
		{
			if (t!=(unsigned int) i) copy.push_back(keyList[t]);
		}
		keyList=copy;
		displayKeys();
	}
}
void Form::setNewPackageCurrentDirectory(QListWidgetItem *item)
{

	prevPackageDirName = currentPackageDir->dirName();
	QFileInfoList list = currentPackageDir->entryInfoList();
	currentPackageDir->cd(list.at(ui.filelistWidget->row(item)+1).fileName());
	reloadPackageDirListing();
	QList<QListWidgetItem *> items;
	items = ui.filelistWidget->findItems(prevPackageDirName, Qt::MatchFixedString | Qt::MatchCaseSensitive);
	if (items.size()>0) ui.filelistWidget->setCurrentItem(items[0]);
}
void Form::reloadPackageDirListing() // Fills the list
{
	setCurrentPathLabel();
	if (!currentPackageDir->exists() || dataType == DATATYPE_NEW) {
		return;
	}
	ui.filelistWidget->clear();
	ui.filelistCurrentPath->setText(currentPackageDir->canonicalPath());
	QFileInfoList list = currentPackageDir->entryInfoList();
	if (list.size()==0) return;
	for (int i=1; i<list.size(); i++) {
		QListWidgetItem *__item = new QListWidgetItem(ui.filelistWidget);
		__item->setText(list.at(i).fileName());
		if (list.at(i).isDir())	__item->setIcon(QIcon("/usr/share/mpkg/packagebuilder/icons/folder.png"));
		else __item->setIcon(QIcon("/usr/share/mpkg/packagebuilder/icons/source.png"));
	}
	if (list.size()>0) ui.filelistWidget->setCurrentRow(0);
}

void Form::setNewFilesystemCurrentDirectory(QListWidgetItem *item)
{

	prevFilesystemDirName = currentFilesystemDir->dirName();
	QFileInfoList list = currentFilesystemDir->entryInfoList();
	currentFilesystemDir->cd(list.at(ui.filelistWidget_2->row(item)+1).fileName());
	reloadFilesystemDirListing();
	QList<QListWidgetItem *> items;
	items = ui.filelistWidget_2->findItems(prevFilesystemDirName, Qt::MatchFixedString | Qt::MatchCaseSensitive);
	if (items.size()>0) ui.filelistWidget_2->setCurrentItem(items[0]);

}
void Form::reloadFilesystemDirListing() // Fills the list
{
	setCurrentPathLabel();

	if (!currentFilesystemDir->exists()) return;
	ui.filelistWidget_2->clear();
	ui.filelistCurrentPath_2->setText(currentFilesystemDir->canonicalPath());
	QFileInfoList list = currentFilesystemDir->entryInfoList();
	if (list.size()==0) return;
	for (int i=1; i<list.size(); i++) {
		QListWidgetItem *__item = new QListWidgetItem(ui.filelistWidget_2);
		__item->setText(list.at(i).fileName());
		if (list.at(i).isDir())	__item->setIcon(QIcon("/usr/share/mpkg/packagebuilder/icons/folder.png"));
		else __item->setIcon(QIcon("/usr/share/mpkg/packagebuilder/icons/source.png"));
	}
	if (list.size()>1) ui.filelistWidget_2->setCurrentRow(0);
}

void Form::goPackageHome()
{
	*currentPackageDir = *packageDir;
	reloadPackageDirListing();
}

void Form::goPackageUp()
{
	currentPackageDir->cdUp();
	reloadPackageDirListing();
}

void Form::goFilesystemHome()
{
	*currentFilesystemDir = QDir::homePath();
	reloadFilesystemDirListing();//DirListing();
}

void Form::goFilesystemUp()
{
	currentFilesystemDir->cdUp();
	reloadFilesystemDirListing();
}
void Form::copyFiles()
{
	manageFiles(FACT_COPY);
}
void Form::manageFiles(FileAction action)
{
	//TODO: fix focus detection
	int direction=focusIndex;
	printf("Direction: %d\n", direction);
	QFileInfo list;
	QFile operatedFile;
	int result=0;
	QString source, dest;
	QList<QListWidgetItem *> itemList;
	QListWidget *widget;
	QDir *curr, *opposite;
	switch(direction)
	{
		case 1: widget = ui.filelistWidget;
			curr = currentPackageDir;
			opposite = currentFilesystemDir;
			break;
		case 2:
			widget = ui.filelistWidget_2;
			curr = currentFilesystemDir;
			opposite = currentPackageDir;
			break;
		default: return;
	}
	itemList = widget->selectedItems();
	printf("itemList size = %d\n", itemList.size());
	for (int i=0; i<itemList.size(); i++) {
		printf("Processing element %d\n",i);
		list = curr->entryInfoList().at(widget->row(itemList[i])+1);
		if (list.fileName()==".." || list.fileName()==".") continue;
		source = list.absoluteFilePath();
		dest = opposite->absolutePath()+"/"+list.fileName();
		printf("Source: [%s]\nDestination: [%s]\n\n", source.toStdString().c_str(), dest.toStdString().c_str());
		if (action==FACT_COPY) {
			//dest = QInputDialog::getText(this, tr("Copy file"), tr("Copy \"") + source + tr("\" to:"), QLineEdit::Normal, dest);
			if (dest.isEmpty()) return;
			result = copyFile(source.toStdString(), dest.toStdString());
		}
		if (action==FACT_MOVE) {
			//dest = QInputDialog::getText(this, tr("Move file"), tr("Move \"") + source + tr("\" to:"), QLineEdit::Normal, dest);
			if (dest.isEmpty()) return;

			result = moveFile(source.toStdString(), dest.toStdString());
		}
		if (action==FACT_REMOVE) {
			if (QMessageBox::question(this, tr("Remove file"), tr("Remove \"") + source + tr("\" ?"), QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)!=QMessageBox::Ok) return;
			result = removeFile(source.toStdString());
		}
		printf("Result: %d\n", result);
	}
	reloadFilesystemDirListing();
	reloadPackageDirListing();
}
void Form::renameFile()
{
	QString newName;
	QFileInfo fileInfo;
	switch(focusIndex)
	{
		case 1:
			fileInfo = currentPackageDir->entryInfoList().at(ui.filelistWidget->currentRow()+1);
			if (fileInfo.fileName()==".." || fileInfo.fileName()==".") return;
			newName = QInputDialog::getText(this, tr("Rename file"), tr("Enter a new file name"), QLineEdit::Normal, fileInfo.fileName());
			if (!newName.isEmpty()) QFile::rename(fileInfo.absoluteFilePath(), currentPackageDir->absolutePath()+"/"+newName);
			break;
		case 2:
			fileInfo = currentFilesystemDir->entryInfoList().at(ui.filelistWidget_2->currentRow()+1);
			if (fileInfo.fileName()==".." || fileInfo.fileName()==".") return;
			newName = QInputDialog::getText(this, tr("Rename file"), tr("Enter a new file name"), QLineEdit::Normal, fileInfo.fileName());
			if (!newName.isEmpty()) QFile::rename(fileInfo.absoluteFilePath(), currentFilesystemDir->absolutePath()+"/"+newName);
			break;
	}
	reloadPackageDirListing();
	reloadFilesystemDirListing();
}
void Form::moveFiles()
{
	manageFiles(FACT_MOVE);
}

void Form::removeFiles()
{
	manageFiles(FACT_REMOVE);
}

void Form::viewFile()
{
	QTextBrowser *browser = new QTextBrowser;
	QFileInfo fileInfo;
	switch(focusIndex)
	{
		case 1:
			fileInfo = currentPackageDir->entryInfoList().at(ui.filelistWidget->currentRow()+1);
			break;
		case 2:
			fileInfo = currentFilesystemDir->entryInfoList().at(ui.filelistWidget_2->currentRow()+1);
			break;
	}
	browser->setText(ReadFile(fileInfo.absoluteFilePath().toStdString()).c_str());
	browser->showMaximized();

}

void Form::editFile()
{
	QFileInfo fileInfo;
	switch(focusIndex)
	{
		case 1:
			fileInfo = currentPackageDir->entryInfoList().at(ui.filelistWidget->currentRow()+1);
			break;
		case 2:
			fileInfo = currentFilesystemDir->entryInfoList().at(ui.filelistWidget_2->currentRow()+1);
			break;
	}
	system("gvim " + fileInfo.absoluteFilePath().toStdString());
}

void Form::makeDirectory()
{
	QString dirName = QInputDialog::getText(this, tr("Make new directory"), tr("Enter new directory name:"));
	if (dirName.isEmpty()) return;
	QDir *curr;
	QListWidget *widget;
	switch(focusIndex) {
		case 1:
			curr = currentPackageDir;
			widget = ui.filelistWidget;
			break;
		case 2:
			curr = currentFilesystemDir;
			widget = ui.filelistWidget_2;
			break;
		default: return;
	}
	curr->mkdir(dirName);
	reloadFilesystemDirListing();
	reloadPackageDirListing();
	QList <QListWidgetItem *> items = widget->findItems(dirName, Qt::MatchFixedString | Qt::MatchCaseSensitive);
	if (items.size()>0) widget->setCurrentItem(items[0]);
}

void Form::importFromDatabase()
{
	string package_name;
	if (ui.NameEdit->text().isEmpty()) {
		package_name = QInputDialog::getText(this, tr("Database query"), tr("Enter a package name:")).toStdString();
	}
	else package_name = ui.NameEdit->text().toStdString();
	printf("Checking for db...\n");
	if (db==NULL) {
		if (!lockDatabase()) {
			QMessageBox::critical(this, tr("Error locking database"), tr("Cannot lock database because it is locked by another process"), QMessageBox::Ok);
			return;
		}
		db = new mpkg;
		printf("db created\n");
	}
	else printf("db NOT CREATED\n");
	PACKAGE_LIST pkgList;
	SQLRecord sqlSearch;
	sqlSearch.addField("package_name", package_name);
	printf("query...\n");
	db->get_packagelist(sqlSearch, &pkgList);
	delete db;
	db=NULL;
	unlockDatabase();
	printf("query complete\n");
	if (pkgList.size()==0) {
		QMessageBox::information(this, tr("Database query results"), tr("No package with name %1 has beed found").arg(package_name.c_str()));
		return;
	}
	if (pkgList.size()>1) {
		// Let's choose the one. Later :)
	}
	printf("setting data\n");
	if (ui.NameEdit->text().isEmpty()) ui.NameEdit->setText(pkgList[0].get_name().c_str());
	if (ui.VersionEdit->text().isEmpty()) ui.VersionEdit->setText(pkgList[0].get_version().c_str());
	if (ui.BuildEdit->text().isEmpty()) ui.VersionEdit->setText(pkgList[0].get_build().c_str());
	ui.ArchComboBox->setEditText(pkgList[0].get_arch().c_str());
	ui.ShortDescriptionEdit->setText(pkgList[0].get_short_description().c_str());
	ui.DescriptionEdit->setText(pkgList[0].get_description().c_str());
	ui.ChangelogEdit->setText(pkgList[0].get_changelog().c_str());
	ui.needSpecialUpdateCheckBox->setChecked(pkgList[0].needSpecialUpdate);
	ui.TagListWidget->clear();
	for (unsigned int i=0; i<pkgList[0].get_tags().size(); i++) {
		ui.TagListWidget->addItem(pkgList[0].get_tags().at(i).c_str());
	}
}
