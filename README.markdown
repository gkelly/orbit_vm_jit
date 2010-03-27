orbit_vm_jit
============
In 2009 I competed with several of my classmates from SE2011 in the ICFP
contest. By 'competed' I should clarify that all I did was write the VM. The
rest of the team actually did the problem-solving goo.

I wrote both of the VMs we ended up using, the first being a relatively slow
naive imeplementation of the spec, and the second being a JIT-compiling VM that
ran each program in about 10% of the time the original version did.

I found this code laying around my home directory, and figured that it should
be remembered somewhere, so here it is. I would have included the original
non-JIT VM as well, but the snapshot of the SVN repo that I have is a heavily
hacked-upon version, and the repo that held the history is long since gone (go
SVN!).

Why not LLVM?
=============
Because it's hideous.

orbit_vm_jit uses libjit, which is _exactly_ what a library should look like.
It seems that it's no longer maintained, which makes me sad.
