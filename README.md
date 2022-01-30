# Remap

This project will copy all pages from the protected process to another process
You will be able to read/write memory, dump the game and so on... without RING0/Windows kernel

I tested it for several hours and no bsod, but it may have that risk.   
It is working for 1803 until 21H2

![imagem](https://user-images.githubusercontent.com/29626806/150711433-8da781b3-3952-4586-8a45-2da3909b2510.png)

    ### Warnings:
    Before closing the game, restore the process you remapped first, otherwise you will have bsod


### Another POC
### It seems that this works too, but it needs some code to make it work perfectly
    typedef NTSTATUS (fastcall* t_MiCloneProcessAddressSpace)(
         IN  PEPROCESS ProcessToClone,
    	    IN  PEPROCESS ProcessToInitialize,
    	    IN  PVOID SectionToMap
     );

    // Win 10 2004 sig
    auto MiCloneProcessAddressSpace = t_MiCloneProcessAddressSpace(FindPatternImage(PVOID(KernelBase), "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ?       ?     ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 1F 45 33 C9 44 89 45 C7 0F 57 C0 4C 89 4D CF 0F 11 45 EF 45 8B F8 48"));
 
     MiCloneProcessAddressSpace(ProcessToClone, ProcessToInitialize, 0); // call function

### https://www.unknowncheats.me/forum/anti-cheat-bypass/487047-remapping-process.html

