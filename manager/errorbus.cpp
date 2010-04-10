/****************************************************************************
 * MOPSLinux packaging system
 * Package manager - error bus thread
 * $Id: errorbus.cpp,v 1.1 2007/05/30 12:51:19 i27249 Exp $
 * *************************************************************************/
#include "corethread.h"
errorBus::errorBus()
{
	action = eBUS_Pause;
	TIMER_RES = 400;
}

void errorBus::run()
{

	//setPriority(QThread::LowestPriority);
	forever
	{
		if (action == eBUS_Run)
		{
			if (getErrorCode()!=MPKG_OK)
			{
				switch(getErrorCode())
				{
					//-------- PACKAGE DOWNLOAD ERRORS ---------//

					case MPKG_DOWNLOAD_OK:
						setErrorReturn(MPKG_RETURN_CONTINUE);
						break;
					case MPKG_DOWNLOAD_TIMEOUT:
					case MPKG_DOWNLOAD_MD5:
					case MPKG_DOWNLOAD_HOST_NOT_FOUND:
					case MPKG_DOWNLOAD_FILE_NOT_FOUND:
					case MPKG_DOWNLOAD_LOGIN_INCORRECT:
					case MPKG_DOWNLOAD_FORBIDDEN:
					case MPKG_DOWNLOAD_OUT_OF_SPACE:
					case MPKG_DOWNLOAD_WRITE_ERROR:

					case MPKG_DOWNLOAD_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Download error"),tr("Some files failed to download. What to do?"),
								QMessageBox::Retry | QMessageBox::Abort | QMessageBox::Ignore, 
								QMessageBox::Retry);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Retry:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							case QMessageBox::Ignore:
								setErrorReturn(MPKG_RETURN_IGNORE);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					//---------------CD-ROM ERRORS/EVENTS---------------------//
					case MPKG_CDROM_WRONG_VOLNAME:
					case MPKG_CDROM_MOUNT_ERROR:
						userReply = QMessageBox::NoButton;
						txt = tr("Please insert disk with label ").toStdString()+CDROM_VOLUMELABEL+tr(" into ").toStdString()+CDROM_DEVICENAME;
						emit sendErrorMessage(tr("Please insert a disk"), \
								txt.c_str(), QMessageBox::Ok | QMessageBox::Abort, QMessageBox::Ok);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Ok:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							default:
								setErrorReturn(MPKG_RETURN_ABORT);
						}
						break;


					//-------- INDEX ERRORS ---------------------------//
					
					case MPKG_INDEX_DOWNLOAD_TIMEOUT:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Download error"),tr("Unable to download repository index. WTF?"),
								QMessageBox::Retry | QMessageBox::Abort | QMessageBox::Ignore, 
								QMessageBox::Retry);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Retry:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							case QMessageBox::Ignore:
								setErrorReturn(MPKG_RETURN_IGNORE);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_INDEX_PARSE_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Parse error"),tr("Error parsing repository index!"),
								QMessageBox::Ok, 
								QMessageBox::Ok);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Ok:
								setErrorReturn(MPKG_RETURN_SKIP);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							case QMessageBox::Ignore:
								setErrorReturn(MPKG_RETURN_IGNORE);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_INDEX_HOST_NOT_FOUND:
						setErrorReturn(MPKG_RETURN_SKIP);
						break;
					case MPKG_INDEX_NOT_RECOGNIZED:
						setErrorReturn(MPKG_RETURN_SKIP);
						break;
					case MPKG_INDEX_LOGIN_INCORRECT:
						setErrorReturn(MPKG_RETURN_SKIP);
						break;
					case MPKG_INDEX_FORBIDDEN:
						setErrorReturn(MPKG_RETURN_SKIP);
					case MPKG_INDEX_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Repository index error"),tr("Error retrieving repository index!"),
								QMessageBox::Ok, 
								QMessageBox::Ok);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Ok:
								setErrorReturn(MPKG_RETURN_SKIP);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;



					//---------- INSTALLATION ERRORS --------------//
					case MPKG_INSTALL_OUT_OF_SPACE:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Out of space!"), tr("Error installing packages - out of space.\nFree some disk space and try again"), QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Retry:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_INSTALL_SCRIPT_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Script error"), tr("Error executing script"), QMessageBox::Ok, QMessageBox::Ok);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						setErrorReturn(MPKG_RETURN_SKIP);
						break;
					case MPKG_INSTALL_EXTRACT_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Package extraction error"), tr("Error extracting package."), QMessageBox::Ok, QMessageBox::Ok);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						setErrorReturn(MPKG_RETURN_SKIP);
						break;

					case MPKG_INSTALL_META_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Error extracting metadata"), tr("Error while extracting metadata from package. Seems that package is broken"), QMessageBox::Ok, QMessageBox::Ok);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						setErrorReturn(MPKG_RETURN_SKIP);
						break;

					case MPKG_INSTALL_FILE_CONFLICT:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("File conflict detected"), tr("Unresolvable file conflict detected. You can force installation, but it is DANGEROUS (it may broke some components)"), QMessageBox::Ignore | QMessageBox::Abort, QMessageBox::Ignore);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Ignore:
								setErrorReturn(MPKG_RETURN_IGNORE);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;



					//---------STARTUP ERRORS---------------//
					case MPKG_STARTUP_COMPONENT_NOT_FOUND:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Some components not found!"), tr("Some components were not found, the program can fail during runtime. Continue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Yes:
								setErrorReturn(MPKG_RETURN_CONTINUE);
								break;
							case QMessageBox::No:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
						}
						userReply = QMessageBox::NoButton;
						break;
					case MPKG_STARTUP_NOT_ROOT:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Access denied"), tr("You should run this program as root"), QMessageBox::Abort, QMessageBox::Abort);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						setErrorReturn(MPKG_RETURN_ABORT);
						break;

					
					//---------- SUBSYSTEM ERRORS ---------------------//
					case MPKG_SUBSYS_SQLDB_INCORRECT:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("SQL database error"),tr("Incorrect database structure. Create from scratch?"),
								QMessageBox::Yes | QMessageBox::No, 
								QMessageBox::No);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Yes:
								setErrorReturn(MPKG_RETURN_REINIT);
								break;
							case QMessageBox::No:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_SUBSYS_SQLDB_OPEN_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("SQL database error"),tr("Unable to open database. This is critical, cannot continue"),
								QMessageBox::Ok,
								QMessageBox::Ok);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						setErrorReturn(MPKG_RETURN_ABORT);
						break;

					case MPKG_SUBSYS_XMLCONFIG_READ_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Error in configuration files"),
							       	tr("Error in configuration files. Try to recreate? WARNING: all your settings will be lost!|"),
								QMessageBox::Yes | QMessageBox::Abort, 
								QMessageBox::Abort);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Yes:
								setErrorReturn(MPKG_RETURN_REINIT);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;
					case MPKG_SUBSYS_XMLCONFIG_WRITE_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Error writing configuration files"),
							       	tr("Error writing configuration files. Retry?"),
								QMessageBox::Yes | QMessageBox::Abort, 
								QMessageBox::Abort);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Yes:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_SUBSYS_SQLQUERY_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Internal error"),
							       	tr("SQL query error detected. This is critical internal error, we exit now."),
								QMessageBox::Abort, 
								QMessageBox::Abort);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_SUBSYS_TMPFILE_CREATE_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Error creating a temporary file"),
							       	tr("Error while creating a temp file. In most cases this mean that no free file descriptors available. This is critical, cannot continue"),
								QMessageBox::Abort, 
								QMessageBox::Abort);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_SUBSYS_FILE_WRITE_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Error writing file"),
							       	tr("File write error! Check for free space. Retry?"),
								QMessageBox::Yes | QMessageBox::Abort, 
								QMessageBox::Abort);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Yes:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;

					case MPKG_SUBSYS_FILE_READ_ERROR:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Error reading file"),
							       	tr("File read error! Retry?"),
								QMessageBox::Yes | QMessageBox::Abort, 
								QMessageBox::Abort);
						while (userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						switch(userReply)
						{
							case QMessageBox::Yes:
								setErrorReturn(MPKG_RETURN_RETRY);
								break;
							case QMessageBox::Abort:
								setErrorReturn(MPKG_RETURN_ABORT);
								break;
							default:
								mError("Unknown reply");
						}
						userReply = QMessageBox::NoButton;
						break;
					default:
						userReply = QMessageBox::NoButton;
						emit sendErrorMessage(tr("Unknown error!!!"), tr("Unknown error occured!!"), QMessageBox::Ignore, QMessageBox::Ignore);
						while(userReply == QMessageBox::NoButton)
						{
							msleep(TIMER_RES);
						}
						setErrorReturn(MPKG_RETURN_IGNORE);
						break;
				}
			}
		}
		
		if (action == eBUS_Stop)
		{
			return;
		}
		msleep(TIMER_RES);
	}
}
void errorBus::receiveUserReply(QMessageBox::StandardButton reply)
{
	userReply = reply;
}

void errorBus::Start()
{
	action = eBUS_Run;
	start();
}

void errorBus::Pause()
{
	action = eBUS_Pause;
}

void errorBus::Stop()
{
	action = eBUS_Stop;
}

