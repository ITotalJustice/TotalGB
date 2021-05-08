#include "filedialog.h"


#ifdef HAS_NFD

#include "nativefiledialog/nfd.h"
#include <string.h>


struct FileDialogResult filedialog_open_file(
	const char* filters
) {
	struct FileDialogResult result = {0};
	nfdchar_t* nfd_out_path = NULL;

	switch (NFD_OpenDialog(filters, NULL, &nfd_out_path)) {
	    case NFD_ERROR:
	    	result.type = FileDialogResultType_ERROR;
	    	break;

	    case NFD_OKAY:
	    	strncpy(result.path, nfd_out_path, sizeof(result.path));
	    	result.type = FileDialogResultType_OK;
	    	break;

	    case NFD_CANCEL:
	    	result.type = FileDialogResultType_CANCEL;
	    	break;
	}

	if (nfd_out_path) {
		NFD_Free(nfd_out_path);
	}

	return result;
}

struct FileDialogResult filedialog_open_folder(
) {
	struct FileDialogResult result = {0};
	nfdchar_t* nfd_out_path = NULL;

	switch (NFD_PickFolder(NULL, &nfd_out_path)) {
	    case NFD_ERROR:
	    	result.type = FileDialogResultType_ERROR;
	    	break;

	    case NFD_OKAY: {
	    	strncpy(result.path, nfd_out_path, sizeof(result.path));
	    	result.type = FileDialogResultType_OK;
	    }	break;

	    case NFD_CANCEL:
	    	result.type = FileDialogResultType_CANCEL;
	    	break;
	}

	if (nfd_out_path) {
		NFD_Free(nfd_out_path);
	}

	return result;
}

struct FileDialogResult filedialog_save_file(
	const char* filters
) {
	struct FileDialogResult result = {0};
	nfdchar_t* nfd_out_path = NULL;

	switch (NFD_SaveDialog(filters, NULL, &nfd_out_path)) {
	    case NFD_ERROR:
	    	result.type = FileDialogResultType_ERROR;
	    	break;

	    case NFD_OKAY:
	    	strncpy(result.path, nfd_out_path, sizeof(result.path));
	    	result.type = FileDialogResultType_OK;
	    	break;

	    case NFD_CANCEL:
	    	result.type = FileDialogResultType_CANCEL;
	    	break;
	}

	if (nfd_out_path) {
		NFD_Free(nfd_out_path);
	}

	return result;
}

#else

#endif
