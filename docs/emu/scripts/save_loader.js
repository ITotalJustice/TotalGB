let save_input = document.getElementById("UploadSaves");
// sanity check to make sure we still have the ID (we might've renamed it!)
if (save_input) {
  save_input.addEventListener("input", OnInput);

  function OnInput() {
    const MAX_SIZE = 0x10000; // 64KiB

    if (save_input.files.length > 0) {
      for (let i = 0; i < save_input.files.length; i++) {
        console.log(
          "name:", save_input.files[i].name,
          "file_size:", save_input.files[i].size
        );

        if (save_input.files[i].size > MAX_SIZE) {
          console.log("save file is too big!");
          return;
        }

        // for some reason, i need to save the value of i, because
        // otherwise ot becomes undefined below when using it with
        // `save_input.files[index].name`...no idea why
        let index = i;
        let reader = new FileReader();

        reader.addEventListener("load", () => {
          let data = new Uint8Array(reader.result);

          // these are the ptr's that are passed the C function.
          let name_array = new Uint8Array(intArrayFromString(save_input.files[index].name));

          let data_ptr = _malloc(data.length * data.BYTES_PER_ELEMENT);
          let name_ptr = _malloc(name_array.length * name_array.BYTES_PER_ELEMENT);
          HEAPU8.set(data, data_ptr);
          HEAPU8.set(name_array, name_ptr);

          // call out rom load function
          ccall('em_upload_save',
            null, // no return (void)
            ['number', 'number', 'number'], // type of the params (ptr is a num, len is a num)
            [name_ptr, data_ptr, data.length] // ptr and len
          );

          // Free memory
          _free(data_ptr);
          _free(name_ptr);
        });

        reader.readAsArrayBuffer(save_input.files[i]);
      }

      // we are going to want to force a sync here!
      FS.syncfs(function (err) {
        if (err) {
          console.log(err);
        }
      });
    }

    else {
      console.log("No saves selected!");
    }
  }
}