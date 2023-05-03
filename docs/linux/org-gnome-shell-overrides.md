# Settings schema 'org.gnome.shell.overrides' is not installed

I recently upgraded to Fedora 38 and Gnome 44, and one of my installed applications give this error message here:
~~~
Settings schema 'org.gnome.shell.overrides' is not installed
~~~

And sure enough, that schema does not exist:
~~~
[akaris@linux schema]$ gsettings list-schemas | grep org.gnome.shell.overrides
[akaris@linux schema]$ 
~~~

I ran into this with a 3rd party application today, and it seems like I am not the only one:
[https://bugs.launchpad.net/ubuntu/+source/gnome-shell-extension-tiling-assistant/+bug/2008139](https://bugs.launchpad.net/ubuntu/+source/gnome-shell-extension-tiling-assistant/+bug/2008139)

I can't fix the 3rd party application that I'm using, but I found a workaround for my case. I'm pretty sure that this workaround isn't complete for all cases, but it did the job for me.

## Root cause analysis

The root cause lies in this commit here:
~~~
$ git show a9e7dfe8fa8717b95f4f451450aa2ac8beab87bc
commit a9e7dfe8fa8717b95f4f451450aa2ac8beab87bc
Author: Florian Müllner <fmuellner@gnome.org>
Date:   Wed Oct 19 15:13:32 2022 +0200

    data: Remove unused overrides schema
    
    The old custom overrides mechanism was superseded by
    session-specific defaults back in 2018. By now any
    potential consumers (like gnome-tweaks) should have
    adjusted, so time to remove it.
    
    Part-of: <https://gitlab.gnome.org/GNOME/gnome-shell/-/merge_requests/2517>

diff --git a/data/org.gnome.shell.gschema.xml.in b/data/org.gnome.shell.gschema.xml.in
index 86ad1b24b..f70d4ea4c 100644
--- a/data/org.gnome.shell.gschema.xml.in
+++ b/data/org.gnome.shell.gschema.xml.in
@@ -316,49 +316,4 @@
       <default>[]</default>
     </key>
   </schema>
-
-  <!-- unused, change 00_org.gnome.shell.gschema.override instead -->
-  <schema id="org.gnome.shell.overrides" path="/org/gnome/shell/overrides/"
-         gettext-domain="@GETTEXT_PACKAGE@">
-    <key name="attach-modal-dialogs" type="b">
-      <default>true</default>
-      <summary>Attach modal dialog to the parent window</summary>
-      <description>
-        This key overrides the key in org.gnome.mutter when running
-        GNOME Shell.
-      </description>
-    </key>
-
-    <key name="edge-tiling" type="b">
-      <default>true</default>
-      <summary>Enable edge tiling when dropping windows on screen edges</summary>
-      <description>
-        This key overrides the key in org.gnome.mutter when running GNOME Shell.
-      </description>
-    </key>
-
-    <key name="dynamic-workspaces" type="b">
-      <default>true</default>
-      <summary>Workspaces are managed dynamically</summary>
-      <description>
-        This key overrides the key in org.gnome.mutter when running GNOME Shell.
-      </description>
-    </key>
-
-    <key name="workspaces-only-on-primary" type="b">
-      <default>true</default>
-      <summary>Workspaces only on primary monitor</summary>
-      <description>
-        This key overrides the key in org.gnome.mutter when running GNOME Shell.
-      </description>
-    </key>
-
-    <key name="focus-change-on-pointer-rest" type="b">
-      <default>true</default>
-      <summary>Delay focus changes in mouse mode until the pointer stops moving</summary>
-      <description>
-        This key overrides the key in org.gnome.mutter when running GNOME Shell.
-      </description>
-    </key>
-  </schema>
 </schemalist>
~~~

Now, I do have a feeling that this commit might be related to this too, but I don't need it for my workaround:
~~~
$ git show a9e6e44ef898671229388938cc3ed511fa394dfc
commit a9e6e44ef898671229388938cc3ed511fa394dfc
Author: Philip Withnall <pwithnall@endlessos.org>
Date:   Mon Jan 23 16:38:36 2023 +0000

    tools: Drop gnome-shell-overrides-migration.sh
    
    The tool was added in 2018 to migrate to per-desktop overrides from the
    old overrides system.
    
    5 years later, everyone who’s going to migrate probably has migrated, so
    we can delete the script and remove a process running on every login.
    
    Signed-off-by: Philip Withnall <pwithnall@endlessos.org>
    Part-of: <https://gitlab.gnome.org/GNOME/gnome-shell/-/merge_requests/2611>
~~~

## Workaround

Let's create the missing `org.gnome.shell.overrides`:
~~~
cat <<'EOF' > org.gnome.shell.overrides.gschema.xml
<?xml version="1.0" encoding="utf-8"?>
<schemalist>
   <!-- unused, change 00_org.gnome.shell.gschema.override instead -->
   <schema id="org.gnome.shell.overrides" path="/org/gnome/shell/overrides/"
          gettext-domain="@GETTEXT_PACKAGE@">
     <key name="attach-modal-dialogs" type="b">
       <default>true</default>
       <summary>Attach modal dialog to the parent window</summary>
       <description>
         This key overrides the key in org.gnome.mutter when running
         GNOME Shell.
       </description>
     </key>
   
     <key name="edge-tiling" type="b">
       <default>true</default>
       <summary>Enable edge tiling when dropping windows on screen edges</summary>
       <description>
         This key overrides the key in org.gnome.mutter when running GNOME Shell.
       </description>
     </key>
   
     <key name="dynamic-workspaces" type="b">
       <default>true</default>
       <summary>Workspaces are managed dynamically</summary>
       <description>
         This key overrides the key in org.gnome.mutter when running GNOME Shell.
       </description>
     </key>
   
     <key name="workspaces-only-on-primary" type="b">
       <default>true</default>
       <summary>Workspaces only on primary monitor</summary>
       <description>
         This key overrides the key in org.gnome.mutter when running GNOME Shell.
       </description>
     </key>
   
     <key name="focus-change-on-pointer-rest" type="b">
       <default>true</default>
       <summary>Delay focus changes in mouse mode until the pointer stops moving</summary>
       <description>
         This key overrides the key in org.gnome.mutter when running GNOME Shell.
       </description>
     </key>
   </schema>
</schemalist>
EOF
sudo cp org.gnome.shell.overrides.gschema.xml /usr/share/glib-2.0/schemas/
~~~

Now, recompile the schemas:
~~~
cd /usr/share/glib-2.0/schemas/
sudo glib-compile-schemas .
~~~

And verify:
~~~
[akaris@linux schema]$ gsettings list-schemas | grep org.gnome.shell.overrides
org.gnome.shell.overrides
~~~

That's it, this fixed my crashing 3rd party app for me.
