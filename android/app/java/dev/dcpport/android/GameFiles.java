package dev.dcpport.android;

import android.content.Context;

import java.io.File;

/** Where the game is, by the rule android_main.c uses: the internal files folder when it holds the boot image, else the
 *  external one (Android/data/&lt;package&gt;/files). */
final class GameFiles {
    private GameFiles() {}

    static File root(Context context) {
        File internal = context.getFilesDir();
        File external = context.getExternalFilesDir(null);
        return installed(internal) || external == null ? internal : external;
    }

    static boolean installed(File root) {
        return root != null && new File(root, "image/v1.03/boot.bin").isFile() && new File(root, "app0").isDirectory();
    }
}
