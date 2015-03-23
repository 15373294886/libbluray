/*
 * This file is part of libbluray
 * Copyright (C) 2015  Petri Hintukainen <phintuka@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */


package org.videolan;

import java.io.FilePermission;
import java.io.File;
import java.util.PropertyPermission;
import java.security.AccessController;
import java.security.Permission;
import java.security.PrivilegedAction;

final class BDJSecurityManager extends SecurityManager {

    private String discRoot;
    private String cacheRoot;
    private String budaRoot;
    private String persistentRoot;
    private boolean usingUdf = false;

    BDJSecurityManager(String discRoot, String persistentRoot, String budaRoot) {
        this.discRoot  = discRoot;
        this.cacheRoot = null;
        this.budaRoot  = budaRoot;
        this.persistentRoot = persistentRoot;
        if (discRoot == null) {
            usingUdf = true;
        }
    }

    protected void setCacheRoot(String root) {
        if (cacheRoot != null) {
            // limit only
            if (!root.startsWith(cacheRoot)) {
                logger.error("setCacheRoot(" + root + ") denied\n" + Logger.dumpStack());
                throw new SecurityException("cache root already set");
            }
        }
        cacheRoot = root;
    }

    /*
     *
     */

    private void deny(Permission perm) {
        logger.error("denied " + perm + "\n" + Logger.dumpStack());
        throw new SecurityException("denied " + perm);
    }

    public void checkPermission(Permission perm) {
        if (perm instanceof RuntimePermission) {
            if (perm.implies(new RuntimePermission("createSecurityManager"))) {
                deny(perm);
            }
            if (perm.implies(new RuntimePermission("setSecurityManager"))) {
                if (classDepth("org.videolan.Libbluray") == 3) {
                    return;
                }
                deny(perm);
            }
        }

        else if (perm instanceof PropertyPermission) {
            // allow read
            if (perm.getActions().equals("read")) {
                String prop = perm.getName();
                if (prop.startsWith("bluray.") || prop.startsWith("dvb.") || prop.startsWith("mhp.") || prop.startsWith("aacs.")) {
                    //logger.info(perm + " granted");
                    return;
                }
                if (prop.startsWith("user.dir")) {
                    //logger.info(perm + " granted\n" + Logger.dumpStack());
                    return;
                }
            }
            try {
                super.checkPermission(perm);
            } catch (Exception e) {
                logger.error(perm + " denied by system");
                deny(perm);
            }
        }

        else if (perm instanceof FilePermission) {
            /* grant delete for writable files */
            if (perm.getActions().equals("delete")) {
                if (canReadWrite(perm.getName())) {
                    return;
                }
                checkWrite(perm.getName());
                return;
            }
        }

        /* Networking */
        else if (perm instanceof java.net.SocketPermission) {
            if (new java.net.SocketPermission("*", "connect,resolve").implies(perm)) {
                return;
            }
        }

        /* Java TV */
        else if (perm instanceof javax.tv.service.ReadPermission) {
            return;
        }
        else if (perm instanceof javax.tv.service.selection.ServiceContextPermission) {
            return;
        }
        else if (perm instanceof javax.tv.service.selection.SelectPermission) {
            return;
        }
        else if (perm instanceof javax.tv.media.MediaSelectPermission) {
            return;
        }

        /* DVB */
        else if (perm instanceof org.dvb.application.AppsControlPermission) {
            return;
        }
        else if (perm instanceof org.dvb.media.DripFeedPermission) {
            return;
        }
        else if (perm instanceof org.dvb.user.UserPreferencePermission) {
            return;
        }

        /* bluray */
        else if (perm instanceof org.bluray.vfs.VFSPermission) {
            return;
        }

        /*
        try {
            java.security.AccessController.checkPermission(perm);
        } catch (java.security.AccessControlException ex) {
            System.err.println(" *** caught " + ex + " at\n" + Logger.dumpStack());
            throw ex;
        }
        */
    }

    /*
     *
     */

    public void checkExec(String cmd) {
        logger.error("Exec(" + cmd + ") denied\n" + Logger.dumpStack());
        throw new SecurityException("exec denied");
    }

    public void checkExit(int status) {
        logger.error("Exit(" + status + ") denied\n" + Logger.dumpStack());
        throw new SecurityException("exit denied");
    }

    public void checkRead(String file) {

        file = getCanonPath(file);

        //super.checkRead(file);
        if (usingUdf) {
            BDJLoader.accessFile(file);
        }
    }

    /*
     * File write access
     */

    private boolean canReadWrite(String file) {
        if (budaRoot != null && file.startsWith(budaRoot)) {
            return true;
        }
        if (persistentRoot != null && file.startsWith(persistentRoot)) {
            return true;
        }
        return false;
    }

    public void checkWrite(String file) {
        BDJXletContext ctx = BDJXletContext.getCurrentContext();

        file = getCanonPath(file);

        if (ctx != null) {
            // Xlet can write to persistent storage and binding unit
            if (canReadWrite(file)) {
                return;
            }
            logger.error("Xlet write " + file + " denied at\n" + Logger.dumpStack());
        } else  {
            // BD-J core can write to cache
            if (cacheRoot != null && file.startsWith(cacheRoot)) {
                return;
            }
            logger.error("BD-J write " + file + " denied at\n" + Logger.dumpStack());
        }


        throw new SecurityException("write access denied");
    }

    private String getCanonPath(final String path)
    {
        String cpath = (String)AccessController.doPrivileged(new PrivilegedAction() {
            public Object run() {
                try {
                    return new File(path).getCanonicalPath();
                } catch (Exception ioe) {
                    logger.error("error canonicalizing " + path + ": " + ioe);
                    return null;
                }
            }
            });
        if (cpath == null) {
            throw new SecurityException("cant canonicalize " + path);
        }
        return cpath;
    }

    /*
     *
     */

    private static final Logger logger = Logger.getLogger(BDJSecurityManager.class.getName());
}
