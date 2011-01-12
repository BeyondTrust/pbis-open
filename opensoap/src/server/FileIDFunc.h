/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: FileIDFunc.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef FILE_ID_FUNC_H
#define FILE_ID_FUNC_H

/* SOAP メッセージに対応するファイル ID を取得する */
extern
std::string
convertToFileID(const std::string& message, const std::string& spoolPath);

/* ファイル ID に対応する SOAP メッセージを取得する */
extern
std::string
revertFromFileID(const std::string& fileID, const std::string& spoolPath);

extern
int
updateFileIDContent(std::string& fileID, 
		    const std::string& message,
		    const std::string& spoolPath);

extern
int
deleteFileID(const std::string& fileID, const std::string& spoolPath);

#endif /* FILE_ID_FUNC_H */
