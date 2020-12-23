---
title: Creating an RPM package for a PHP PEAR module
date: 2014-12-17 12:19:38
tags: [ Fedora, Packaging ]
categories: Opensource
---
I am participating in Google Code In 2014 and two of my tasks were based on RPM packaging for Fedora.  
[The first](http://www.google-melange.com/gci/task/view/google/gci2014/5262603731337216) was packaging [PhalconPHP](http://www.phalconphp.com/en/) for Fedora while [the second](http://www.google-melange.com/gci/task/view/google/gci2014/5774064475963392) was packaging [CakePHP](http://cakephp.org/).  
Phalcon is an C extension while Cake is a PEAR/Composer extension.

<!-- more -->

I will discuss the easier of the two, Cake as it was a PEAR package.

So first of all, you need to learn some RPM Packaging Basics. [This link](https://fedoraproject.org/wiki/How_to_create_an_RPM_package) briefly introduces us to the necessary tools and their setup. But the above link does not introduce us to [Copr](http://copr.fedoraproject.org/) which may be used to distribute and even build our RPMS on different platforms (we can use it as a sort of substitute for [Koji or Mock](https://fedoraproject.org/wiki/How_to_create_an_RPM_package#Mock_and_Koji)).

Then I recommend that you read [PHP Packaging wiki]([http://fedoraproject.org/wiki/Packaging](http://fedoraproject.org/wiki/Packaging):

## The Channel package

So lets say we have identified the PEAR channel (pear.example.org) and it is not the standard PEAR channel. So we need to create a CHANNEL package.

So lets create the SPEC file :
{% gist meghprkh/066d9477881f168c77b4 %}

As you can see you just need to edit a few lines (lines 2,3) to update the channel name and URL and the last lines for proper changelogs.

Now you can build this spec using the `rpmbuild` command.

Install the generated RPM on your system (provided the build is successful).

## The PEAR Package

Now lets head on to creating the actual SPEC file of the RPM package.

*   First install the `php-pear-PEAR-Command-Packaging` package.
*   Download the PEAR package from the channel (mostly _[http://channelname/get/name-version.tgz](http://channelname/get/name-version.tgz)_)
*   Run `pear make-rpm-spec Foo.tgz` .
*   and a spec file will be generated for you (with all required files) …
*   … but if it is not from the standard PEAR channel, you will need to add the channel to the requires and modify certain lines …


*   You may look at [my spec for CakePHP](https://gist.github.com/meghprkh/39fa65e683f36a4b3996)
*   Done now You just need to build it …
*   If the build fails make sure you had installed the Channel Package

```
%global pear_name example
%global pear_channel pear.example.org
URL:            http://%{pear_channel}/package/%{pear_name}
Source0:        http://%{pear_channel}/get/%{pear_name}-%{version}.tgz
BuildRequires:  php-channel(%{pear_channel})
Requires:       php-channel(%{pear_channel})
```

## The Copr Build System

Lets say you have Fedora 20 on your system but you also want to build it for other Fedora and EPEL versions and also make the installation simpler for the end user. So the right tool you need to use is the [Copr](http://copr.fedoraproject.org/) Build System.

*   Upload your SRPMs to some file sharing service which permits direct downloads through a link (I used GitHub but thats a bad habit for git is extremely slow with binary files (SRPMS are gzipped files)).
*   Go to the Copr build system page.
*   Read their wiki a little.
*   Login and create a new repo
*   First build the Channel package in your repo
*   Drink some coffee. Its gonna take some time.
*   Next build the PEAR package (build it after the first completes for it is dependent onthe channel package)
*   Share your work.
*   You may see my repo for [copy and paste installation instructions](https://copr.fedoraproject.org/coprs/meghprkh/cakephp/)
