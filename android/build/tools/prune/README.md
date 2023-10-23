# Tool to check if a branch can be pruned

This tool checks if all the commits with a Change-Id on a specific branch occur on a main branch. If this condition is met, the branch can be safely deleted. Typically, this situation indicates that your commit successfully passed through Gerrit and was merged into the emu-master-dev branch.

This tool is particularly useful when there is an accumulation of branches with changes that have been integrated into the mainline, yet the 'repo prune' command fails to detect that the branches can be safely deleted.

For example:

```
12006 ± emu-prune prune delete-me⏎
Using aosp/emu-master-dev as the reference branch
delete-me can be deleted.
```

Next you can auto delete this branch with the `--delete` flag.

```
12009 ± emu-prune --delete prune delete-me⏎
Using aosp/emu-master-dev as the reference branch
delete-me can be deleted.
Deleted branch delete-me (was 3ec3ebea).
The local branch 'delete-me' has been successfully deleted.
```

You can also try to prune all the branches:

```
12013 ± emu-prune prune-all⏎
Using aosp/emu-master-dev as the reference branch
Checking branch bad_resize.
Unable to delete branch bad_resize since Change-Id: Ibd9da9ca943aa21f980a0fc46b8b7bc232d1ef34 (sha: f116de82941ece67c7d6d4def6690523b29e3cd9) is missing in aosp/emu-master-dev
Checking branch disp_changes.
Unable to delete branch disp_changes since Change-Id: I4f7a7be958ffeb07f9a0dceea50cdf669936f8a3 (sha: d7f74d7d3ede8deac8d3c2f88adab766df5a768c) is missing in aosp/emu-master-dev
Checking branch fix-doc.
Unable to delete branch fix-doc since Change-Id: I48093f809ba0a41e4b39c4287140f55631e3f530 (sha: cd0df468405d12ab22005e8fa32d432088a34ba9) is missing in aosp/emu-master-dev
Checking branch fix-log.
```

Again with the `--delete` flag you can automatically delete branches that have been merged.


## Additional Notes
By default, the tool will use aosp/emu-master-dev (the current development branch) as the reference branch for checking merges. You can change this by providing the --main parameter.

For example:

```
▶ emu-prune --main emu-34-release prune-all
Using emu-34-release as the main branch
Checking branch adb-log.
```

Please note that this tool will likely not work if you are not in a repo of that branch.