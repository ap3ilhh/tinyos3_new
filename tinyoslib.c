

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio_ext.h>

#include "util.h"
#include "tinyos.h"
#include "tinyoslib.h"



static ssize_t tinyos_fid_read(void *cookie, char *buf, size_t size)
{
	return Read(*(Fid_t*)cookie, buf, size); 
}

static ssize_t tinyos_fid_write(void *cookie, const char *buf, size_t size)
{
#if 1 
	/* There is a bug in the custom stream code where it is assumed that all
	   the bytes given will be written. Therefore, we cannot simply pass the
	   arguments to Write(). Instead, we should write as much as we can, until 
	   error! 
	*/
	Fid_t fid = *(Fid_t*)cookie;
	int nbytes = 0;
	while(nbytes<size) {
		int ret = Write(fid, buf+nbytes, size-nbytes);
		if(ret<0) break;
		nbytes += ret;
	}
	return nbytes;
#else
	int ret = Write(*(Fid_t*)cookie, buf, size);
	return (ret<0) ? 0 : ret;
#endif
}


static int tinyos_fid_seek(void* cookie, off64_t* position, int whence)
{
	Fid_t fid = *(Fid_t*)cookie;
	intptr_t pos = Seek(fid, *position, whence);
	if(pos<0) return -1;
	*position = pos;
	return 0;
}


static int tinyos_fid_close(void* cookie)
{
	free(cookie);
	return 0;
}

static cookie_io_functions_t  tinyos_fid_functions =
{
	tinyos_fid_read,
	tinyos_fid_write,
	tinyos_fid_seek,
	tinyos_fid_close
};

static FILE* get_std_stream(int fid, const char* mode)
{
	FILE* term = fidopen(fid, mode);
	assert(term);
	/* This is glibc-specific and tunrs off fstream locking */
	__fsetlocking(term, FSETLOCKING_BYCALLER);	
	return term;
}


FILE* fidopen(Fid_t fid, const char* mode)
{

	Fid_t* fidloc = (Fid_t *) malloc(sizeof(Fid_t));
	* fidloc = fid;
	FILE* f = fopencookie(fidloc, mode, tinyos_fid_functions);

	CHECKRC(setvbuf(f, NULL, _IONBF, 0));
	return f;
}

FILE *saved_in = NULL, *saved_out = NULL;


void tinyos_replace_stdio()
{
	assert(saved_in == NULL);
	assert(saved_out == NULL);
	//if(GetTerminalDevices()==0) return;

	FILE* termin = get_std_stream(0, "r");
	FILE* termout = get_std_stream(1, "w");

	saved_in = stdin;
	saved_out = stdout;

	stdin = termin;
	stdout = termout;
}

void tinyos_restore_stdio()
{
	if(saved_out == NULL)  return;	

	fclose(stdin);
	fclose(stdout);

	stdin = saved_in; 
	stdout = saved_out;

	saved_in = saved_out = NULL;
}



static int exec_wrapper(int argl, void* args)
{
	/* unpack the program pointer */
	Program prog;

	/* unpack the prog pointer */
	memcpy(&prog, args, sizeof(prog));

	argl -= sizeof(prog);
	args += sizeof(prog);

	/* unpack the string vector */
	size_t argc = argscount(argl, args);
	const char* argv[argc];
	argvunpack(argc, argv, argl, args);

	/* Make the call */
	return prog(argc, argv);
}


int ParseProcInfo(procinfo* pinfo, Program* prog, int argc, const char** argv )
{
	if(pinfo->main_task != exec_wrapper)
		/* We do not recognize the format! */
		return -1;

	if(pinfo->argl > PROCINFO_MAX_ARGS_SIZE) 
		/* The full argument is not available */
		return -1;

	int argl = pinfo->argl;
	void* args = pinfo->args;

	/* unpack the program pointer */
	if(prog) memcpy(&prog, args, sizeof(prog));

	argl -= sizeof(Program*);
	args += sizeof(Program*);

	/* unpack the string vector */
	size_t N = argscount(argl, args);
	if(argv) {
		if(argc>N)
			argc = N;
		argvunpack(argc, argv, argl, args);
	}

	return N;		
}



int Execute(Program prog, size_t argc, const char** argv)
{
	/* We will pack the prog pointer and the arguments to 
	  an argument buffer.
	  */

	/* compute the argument buffer size */
	size_t argl = argvlen(argc, argv) + sizeof(prog);

	/* allocate the buffer */
	char args[argl];

	/* put the pointer at the start */
	memcpy(args, &prog, sizeof(prog));

	/* add the string vector */
	argvpack(args+sizeof(prog), argc, argv);

	/* Execute the process */
	return Spawn(exec_wrapper, argl, args);
}


Fid_t Dup(Fid_t oldfid)
{
	Fid_t newfid = OpenNull();
	if(newfid!=NOFILE) {
		if(Dup2(oldfid, newfid)==0) 
			return newfid;
		else {
			Close(newfid);
			return NOFILE;
		}
	} 
	else return NOFILE;
}



void BarrierSync(barrier* bar, unsigned int n)
{
	assert(n>0);
	Mutex_Lock(& bar->mx);

	int epoch = bar->epoch;

	bar->count ++;
	if(bar->count >= n) {
		bar->epoch ++;
		bar->count = 0;
		Cond_Broadcast(&bar->cv);
	}

	while(epoch == bar->epoch)
		Cond_Wait(&bar->mx, &bar->cv);

	Mutex_Unlock(& bar->mx);
}


int ReadDir(int dirfd, char* buffer, unsigned int size)
{
	char nbuf[3];

	for(int i=0; i<2; ) {
		int rc = Read(dirfd, nbuf+i, 2-i);
		if(rc==0) return 0;
		if(rc==-1) return -1;
		i+=rc;
	}
	nbuf[2] = '\0';

	int len = strtol(nbuf, NULL, 16)+1;
	if(len>size) {
		Seek(dirfd, -2, SEEK_END);
		return -1;
	}

	for(int i=0; i<len; ) {
		int rc = Read(dirfd, buffer+i, len-i);
		if(rc==0) return 0;
		if(rc==-1) return -1;
		i+=rc;
	}
	return len;
}
