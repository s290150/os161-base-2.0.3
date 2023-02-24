La password dello user os161user e': os161user
Per modificarla, usare comando passwd oppure Settings->Users

Per eseguire os161 occorre andare nella cartella os161/root, da dove eseguire, ad esempio: sys161 kernel

I sorgenti di os161 sono nelle cartelle con radice os161/os161-base-2.0.3/kern

Per configurare e attivare una cartella di scambio tra guest e host in vbox
- attivare la cartella dall'host: dispositivi -> cartelle condivise ... scegliere una cartella di passaggio (una cartella del sistema host) a cui dare un nome: ad esempio "vboxshared"
- In ubuntu, in una finestra "terminal", eseguire:
sudo mount -t vboxsf vboxshared /vboxshared (occorre la password: os161user)
La cartella /vboxshared e' quella condivisa.

==============================================================================

The password of os161user is: os161user
To change it, use the "passwd" command or go to Settings-> Users

To run os161 you need to go to the os161/root folder, from where to run, for example: sys161 kernel

The os161 sources are in the os161/os161-base-2.0.3/kern directory

To configure and activate an exchange folder between guest and host in vbox
- activate the folder from the host: devices -> shared folders ... choose a pass folder (a folder on the host system): for example "vboxshared"
- In ubuntu, in a "terminal" window, execute:
sudo mount -t vboxsf vboxshared/vboxshared (password required: os161user)
The /vboxshared folder is the shared one.

