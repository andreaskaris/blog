## Issues with Dell iDrac and workaround

Since my upgrade to Fedora 33, some of our iDrac servers wouldn't accept keyboard input any more.
~~~
[akaris@linux cases]$ /usr/libexec/icedtea-web/javaws viewer.jnlp 
selected jre: /usr/lib/jvm/jre-11-openjdk
WARNING: package javax.jnlp not in java.desktop
Fri Feb 19 18:32:21 CET 2021 [INFO   ] net.sourceforge.jnlp.runtime.EnvironmentPrinter: OpenWebStartLauncher called with args: [viewer.jnlp].
Fri Feb 19 18:32:21 CET 2021 [INFO   ] net.sourceforge.jnlp.runtime.EnvironmentPrinter: OS: Linux
Fri Feb 19 18:32:21 CET 2021 [INFO   ] net.sourceforge.jnlp.runtime.EnvironmentPrinter: Java Runtime Red Hat, Inc.-11.0.10
(...)

 Client random: 1741975
 KVM session state SESSION_RUNNING
 KVM session is running.
Exception in thread "AWT-EventQueue-1" java.lang.NoSuchMethodError: 'java.awt.peer.ComponentPeer java.awt.Window.getPeer()'
	at com.avocent.kvm.nativekeyboard.NativeKVM.a(Unknown Source)
	at com.avocent.kvm.nativekeyboard.b.a(Unknown Source)
	at com.avocent.kvm.nativekeyboard.b.b(Unknown Source)
	at com.avocent.kvm.nativekeyboard.b.windowOpened(Unknown Source)
	at java.desktop/java.awt.AWTEventMulticaster.windowOpened(AWTEventMulticaster.java:348)
	at java.desktop/java.awt.Window.processWindowEvent(Window.java:2075)
	at java.desktop/javax.swing.JFrame.processWindowEvent(JFrame.java:298)
	at java.desktop/java.awt.Window.processEvent(Window.java:2037)
	at java.desktop/java.awt.Component.dispatchEventImpl(Component.java:5011)
	at java.desktop/java.awt.Container.dispatchEventImpl(Container.java:2321)
	at java.desktop/java.awt.Window.dispatchEventImpl(Window.java:2772)
	at java.desktop/java.awt.Component.dispatchEvent(Component.java:4843)
	at java.desktop/java.awt.EventQueue.dispatchEventImpl(EventQueue.java:772)
	at java.desktop/java.awt.EventQueue$4.run(EventQueue.java:721)
	at java.desktop/java.awt.EventQueue$4.run(EventQueue.java:715)
	at java.base/java.security.AccessController.doPrivileged(Native Method)
	at java.base/java.security.ProtectionDomain$JavaSecurityAccessImpl.doIntersectionPrivilege(ProtectionDomain.java:85)
	at java.base/java.security.ProtectionDomain$JavaSecurityAccessImpl.doIntersectionPrivilege(ProtectionDomain.java:95)
	at java.desktop/java.awt.EventQueue$5.run(EventQueue.java:745)
	at java.desktop/java.awt.EventQueue$5.run(EventQueue.java:743)
	at java.base/java.security.AccessController.doPrivileged(Native Method)
	at java.base/java.security.ProtectionDomain$JavaSecurityAccessImpl.doIntersectionPrivilege(ProtectionDomain.java:85)
	at java.desktop/java.awt.EventQueue.dispatchEvent(EventQueue.java:742)
	at java.desktop/java.awt.EventDispatchThread.pumpOneEventForFilters(EventDispatchThread.java:203)
	at java.desktop/java.awt.EventDispatchThread.pumpEventsForFilter(EventDispatchThread.java:124)
	at java.desktop/java.awt.EventDispatchThread.pumpEventsForHierarchy(EventDispatchThread.java:113)
	at java.desktop/java.awt.EventDispatchThread.pumpEvents(EventDispatchThread.java:109)
	at java.desktop/java.awt.EventDispatchThread.pumpEvents(EventDispatchThread.java:101)
	at java.desktop/java.awt.EventDispatchThread.run(EventDispatchThread.java:90)
 Video connect status: true
CoreSessionListener : state running
in CoreSessionListner : fireOnSessionStateChanged 
APCP 8100 status recvd. Status: 0
 KVM session state SESSION_RUNNING
 KVM session is running.
in CoreSessionListner : fireOnSessionStateChanged 
 KVM session state SESSION_RUNNING
 KVM session is running.
process() DisplayResolutionResponse: resolution change [1024x768]
 Call to resize from outside event thread.
** Max Size: W = 1920 H = 1053
 Call to resize from outside event thread.
** Window Pref Size: W = 1024 H = 841
** Max Size: W = 1920 H = 1053
** Window Pref Size: W = 1024 H = 841
Boot Device: Local:
   Resource Key: NextBoot_0
   Resource Key: NextBoot_1
   Resource Key: NextBoot_6
   Resource Key: NextBoot_15
   Resource Key: NextBoot_5
   Resource Key: NextBoot_2
   Resource Key: NextBoot_7
   Resource Key: NextBoot_8
   Resource Key: NextBoot_16
   Resource Key: NextBoot_18
   Resource Key: NextBoot_17
   Resource Key: NextBoot_19
~~~

I installed Adopt JDK 8 to fix this: https://adoptopenjdk.net/installation.html?variant=openjdk8&jvmVariant=hotspot#x64_linux-jdk

Execute the following command to configure icdetea:

~~~
$ /usr/libexec/icedtea-web/itweb-settings
~~~

Adopt openJDK in JVM Settings and connect with openjdk.

See: https://translate.google.com/translate?sl=auto&tl=en&u=https://minimonk.net/10111
