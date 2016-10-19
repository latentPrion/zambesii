- [Prerequisites](#orgheadline1)
- [Get the Source](#orgheadline2)
- [Ready!](#orgheadline5)
  - [ZUDI](#orgheadline3)
  - [Zambesii](#orgheadline4)
- [Boot](#orgheadline6)


# Prerequisites<a id="orgheadline1"></a>

-   Git
-   A GCC cross-compiler configured for i586-elf target
-   cdrtools or cdrkit

For the BSDs and other \*nixen you'll also need GNU Make and Bash.

Set your PATH to include where you built the cross-compiler:

    export PATH="~/toolchain/bin:$PATH"

That's all! Zambesii has no other dependencies and is very easy to build.

# Get the Source<a id="orgheadline2"></a>

There are two parts to the Zambesii source: Zambesii and ZUDI. Clone their repositiories:

    git clone https://github.com/latentPrion/zambesii
    git clone https://github.com/latentPrion/zudi

# Ready!<a id="orgheadline5"></a>

## ZUDI<a id="orgheadline3"></a>

ZUDI is a small set of utilities that Zambesii uses to build.

Note: substitute `gmake` in place of `make` if you're on the BSDs or another system with a default non-GNU Make.

    cd ~/zudi   # Or whever you cloned ZUDI to
    make
    sudo make install

That's all there is to it.

## Zambesii<a id="orgheadline4"></a>

Now that you have all the dependencies installed, building Zambesii is a snap. No mucking around with dependencies or autotools necessary.

Start with `./configure` and answer ‘gnu’ to the toolchain questions. For the architecture questions, just enter the only option it gives (since Zambesii is x86-only currently). When you get to the toolchain target question, enter i586-elf.

Then simply

    make

And you're done!

# Boot<a id="orgheadline6"></a>

After the ISO is made, you can test that it boots with

    qemu-system-i386 -boot d -cdrom zambesii.iso -m 256M

NUMA support can be tested with QEMU as well:

    qemu-system-i386 -boot d -cdrom zambesii.iso -m 1G -smp 8 -numa node,mem=512M,cpus=0-3 -numa node,mem=256M,cpus=4-5 -numa node,mem=256M,cpus=6-7
