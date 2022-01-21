package org.libsdl.totalgb;

import android.content.Intent;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import androidx.documentfile.provider.DocumentFile;

import org.libsdl.app.SDLActivity;

import java.io.IOException;


public class totalgbActivity extends SDLActivity {
    private static final int REQUEST_CODE_ROM = 0;
    private static final int REQUEST_CODE_SAVE = 1;
    private static final int REQUEST_CODE_STATE = 2;
    private static final int REQUEST_CODE_ZIP_EXPORT = 3;

    public void openFile(int request_code) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, request_code);
    }

    public void openSettings(int request_code) {
        Intent intent = new Intent(totalgbActivity.this, SettingsActivity.class);
        startActivity(intent);
    }

    public void createFile(int request_code) {
        Intent intent = null;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT) {
            intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, request_code);
        }
    }

    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        super.setOrientationBis(w, h, resizable, hint);
    }

    @Override
    protected void onActivityResult(int request_code, int result_code, Intent data) {
        super.onActivityResult(result_code, result_code, data);

        if (result_code != RESULT_OK) {
            Log.d("SDL/APP", "onActivityResult: failed file picker: " + result_code);
            return;
        }

        switch (request_code) {
            case REQUEST_CODE_ROM:
                try {
                    final Uri uri = data.getData();
                    final DocumentFile doc_file = DocumentFile.fromSingleUri(this, uri);
                    if (doc_file != null) {
                        final String file_name = doc_file.getName();
                        try (ParcelFileDescriptor fd = getContentResolver().openFileDescriptor(uri, "r")) {

                            Log.d("SDL/APP", "file_name: " + file_name);
                            // using get returns the fd int, but the parcelFile still owns it!
                            // therefore, should not be closed by callback.
                            // detach fd will give up ownership, though i chose the former.
                            OpenFileCallback(fd.getFd(), false, file_name);
                        }
                    }
                }
                catch (IOException e) {
                    Log.d("SDL/APP", "got exception when trying to open file " + e.getMessage());
                }
                break;
            default:
                throw new IllegalStateException("Unexpected value: " + request_code);
        }
    }

    native void OpenFileCallback(int fd, boolean own_fd, String file);
}
