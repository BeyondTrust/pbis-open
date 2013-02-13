/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseNativeAdapter.java
 *
 * Abstract:
 *
 *        Likewise Common Library
 *        
 *        Likewise Native Adapter (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import java.io.File;

/**
 * Base class used to load Likewise Native Libraries using Java Native Access.
 * <p>
 * This class searches for the library in "/opt/likewise/lib64" and then in
 * "/opt/likewise/lib" in that order and returns with the first instance found.
 * </p>
 */
public class LikewiseNativeAdapter
{
    static
    {
        File lib_dir = new File("/opt/likewise/lib64");
        
        if (lib_dir.exists() && lib_dir.isDirectory())
        {
            System.setProperty("jna.library.path", "/opt/likewise/lib64");
        }
        else
        {
            System.setProperty("jna.library.path", "/opt/likewise/lib");
        }
        
        System.setProperty("jna.wchar-size", "2");
    }
}
