// This is our javascript, it's role is to attach an event to the filepicker
// so that when a file is selected, a function callback will be called.

// This callback will load the rom as an array of bytes, and then pass that data,
// along with it's len, to the C rom loading function!


let rom_input = document.getElementById("RomFilePicker");

// sanity check to make sure we still have the ID (we might've renamed it!)
if (rom_input) {
  console.log("input happened!!!");

  rom_input.addEventListener("input", OnInput);
}

else {
  console.log("Invalid file!");
}


function OnInput() {
  // this is 4MiB-ish
  // this size was chosen because the largest official game was
  // 4MiB (shantae).
  // when compressed as a zip, the size will most often be much
  // smaller, unless store was selected, in which case, due
  // to the zip header, the size will be slightly larger
  // so the size below is adjusted slightly.
  // the compressed size is then handled in the app anyway
  // so malicious .zips won't blow up the mem usage.
  const MAX_SIZE = 0x410000;

  if (rom_input.files.length == 1 && rom_input.files[0].size <= MAX_SIZE) {
    console.log(
      "name:", rom_input.files[0].name,
      "num files:", rom_input.files.length, 
      "file_size:", rom_input.files[0].size
    );

    let reader = new FileReader();

    reader.addEventListener("load", () => {
      let data = new Uint8Array(reader.result);

      // the js string needs to be converted to an int_array
      // however, setting the heapu8 data with that int array
      // seems to do very strange things.
      // printf() prints the string fine, but the string itself
      // is all wrong, the chars are messed up and repeat.
      // idk why this is, but creating a u8 array and copying the data
      // to that instead seems to work...again, no idea why.
      let name_array = new Uint8Array(intArrayFromString(rom_input.files[0].name));

      let data_ptr = _malloc(data.length * data.BYTES_PER_ELEMENT);
      let name_ptr = _malloc(name_array.length * name_array.BYTES_PER_ELEMENT);
      HEAPU8.set(data, data_ptr);
      HEAPU8.set(name_array, name_ptr);

      // call out rom load function
      ccall('em_load_rom_data',
        null, // no return (void)
        ['number', 'number', 'number'], // type of the params (ptr is a num, len is a num)
        [name_ptr, data_ptr, data.length] // ptr and len
      );

      // Free memory
      _free(data_ptr);
      _free(name_ptr);
    });

    reader.readAsArrayBuffer(rom_input.files[0]);
  }

  else {
    console.log("something wrong with the file!");
  }
}