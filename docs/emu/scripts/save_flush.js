// this script preiodically flushes the save file of the game!

const TEN_SECONDS = 10000;
setInterval(OnTimeOut, TEN_SECONDS);

function OnTimeOut() {
	ccall('em_flush_save',
	  null, // no return (void)
	  null,
	  null
	);
}