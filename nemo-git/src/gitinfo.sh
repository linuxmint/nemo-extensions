#!/bin/bash
# Shell script for git-nemo calling git binaries to retrieve toplevel,
# branch and file info status of the current directory.
# .
# Copyright (C) 2015  Eduard Dopler <kontakt@eduard-dopler.de>
# .
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# .
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# .
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

git rev-parse --show-toplevel
git status -b -z
