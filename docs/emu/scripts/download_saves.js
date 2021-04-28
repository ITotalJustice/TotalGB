// this is a very quick an hacky way to download ALL saves
// in a zip file.

// needed to quickly make this as i deleted all
// my gbc saves from my pc, thankfully, they were still
// there in the indexeddb, bur for some reason, i cannot just
// download the files from chrome-dev-tools...

let dl_input = document.getElementById("DlSaves");

// sanity check to make sure we still have the ID (we might've renamed it!)
if (dl_input) {
  dl_input.addEventListener("click", OnInput);
}
else {
  console.log("[JS] couldn't input get element by ID!");
}

function OnInput() {
	// we call 2 "C" functions,
	// 1 to start zipping and it returns the size.
	// if size is > 0, then the second functions is
	// called which returns a ptr to the memory.

	let data_size = _em_zip_all_saves();

	if (data_size == 0) {
		console.log("[JS] zip size was zero from get all saves");
		return;
	}

	let data_ptr = ccall('em_get_zip_saves_data', 'number', []);

	// transform data into js data
	let js_data = new Uint8Array(HEAPU8.subarray(data_ptr, data_ptr + data_size));

	// transform js data to blob, this can be downloaded!
	let blob = new Blob([js_data]);

	// update the download URL
	let DlSaves = document.getElementById("DlSaves");
	DlSaves.href = URL.createObjectURL(blob);

	// we need to free the mem allocated from "C" when done
	_free(data_ptr);

	console.log("[JS] DlSaves Done!");
}
