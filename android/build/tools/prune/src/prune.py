import argparse
import functools
import logging
from tqdm import tqdm
import subprocess
from log import configure_logging

# # Configure the logging format
# logging.basicConfig(format="%(message)s", level=logging.INFO)


class MissingChangeError(Exception):
    def __init__(self, change_id, sha):
        self.change_id = change_id
        self.sha = sha
        super().__init__(f"Change {change_id} missing at commit {sha}")


@functools.lru_cache(maxsize=None)
def get_commit_order(commit_hash_a, commit_hash_b):
    """Get the correct order of two commit hashes.

    This function uses the 'git merge-base --is-ancestor' command to determine
    the correct order of the two commit hashes.

    Args:
        commit_hash_a (str): The first commit hash.
        commit_hash_b (str): The second commit hash.

    Returns:
        tuple: A tuple containing the correct order of the two commit hashes.

    Raises:
        subprocess.CalledProcessError: If the 'git merge-base' command encounters an error.
    """

    result = subprocess.run(
        ["git", "merge-base", "--is-ancestor", commit_hash_a, commit_hash_b],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode == 0:
        logging.debug("Order: %s < %s.", commit_hash_a, commit_hash_b)
        return commit_hash_a, commit_hash_b
    else:
        logging.debug("Order: %s < %s.", commit_hash_b, commit_hash_a)
        return commit_hash_b, commit_hash_a


def count_commits_between(commit_hash_a, commit_hash_b):
    """
    Count the number of commits between two commit hashes.

    Args:
        commit_hash_a (str): The first commit hash.
        commit_hash_b (str): The second commit hash.

    Returns:
        int: The number of commits between the two specified commit hashes.

    """
    try:
        older, newer = get_commit_order(commit_hash_a, commit_hash_b)
        result = subprocess.run(
            ["git", "rev-list", older + ".." + newer],
            capture_output=True,
            text=True,
            check=True,
        )
        commit_count = len(result.stdout.strip().split("\n"))
        return commit_count
    except subprocess.CalledProcessError as e:
        logging.error("An error occurred: %s", e)
        return None


@functools.lru_cache(maxsize=None)
def get_change_ids(commit_hash_start, commit_hash_end):
    """Get the Change-Id values between two commit hashes.

    This function retrieves the Change-Id values from the commit messages
    between the two specified commit hashes.

    Args:
        commit_hash_start (str): The starting commit hash.
        commit_hash_end (str): The ending commit hash.

    Returns:
        list: A list of tuples containing the Change-Id values and the commit hash where the Change-Id is present.

    Raises:
        ValueError: If the number of Change-Id values doesn't match the number of commits between the two hashes.
    """
    logging.debug(
        "Getting change ids for commits %s and %s.", commit_hash_start, commit_hash_end
    )
    older, newer = get_commit_order(commit_hash_start, commit_hash_end)
    logging.debug(
        "Executing command: git log %s..%s --grep=Change-Id: --format=%%H %%n%%B",
        older,
        newer,
    )
    result = subprocess.run(
        ["git", "log", older + ".." + newer, "--grep=Change-Id:", "--format=%H %n%B"],
        capture_output=True,
        text=True,
        check=True,
    )
    commit_info = result.stdout.split("\n")
    change_ids = []
    current_commit = commit_info[0].strip()

    for info in commit_info:
        if info.startswith("Change-Id:"):
            change_id = info.split("Change-Id: ")[-1]
            change_ids.append((change_id, current_commit))

    if len(change_ids) != count_commits_between(older, newer):
        logging.debug("Not every commit has a Change-Id!")
    return change_ids


def find_last_common_commit(branch_a, branch_b):
    """Find the last common commit between two branches.

    This function uses the 'git merge-base' command to determine the last common commit between two branches.

    Args:
        branch_a (str): The first branch.
        branch_b (str): The second branch.

    Returns:
        str: The commit hash representing the last common commit between the two branches.
    """
    logging.debug(
        "Finding last common commit for branches %s and %s.", branch_a, branch_b
    )
    result = subprocess.run(
        ["git", "merge-base", branch_a, branch_b],
        capture_output=True,
        text=True,
        check=True,
    )
    common_commit = result.stdout.strip()

    return common_commit


def check_change_id_exists(change_id, commit_hash_a, commit_hash_b):
    """Check if a Change-Id exists between two commit hashes.

    This function checks if the given Change-Id exists between two commit hashes.

    Args:
        change_id (str): The Change-Id to search for.
        commit_hash_a (str): The first commit hash.
        commit_hash_b (str): The second commit hash.

    Returns:
        bool: True if the Change-Id exists between the commit hashes, otherwise False.
    """
    older, newer = get_commit_order(commit_hash_a, commit_hash_b)
    logging.debug(
        "Checking change ID %s exists between [%s, %s].",
        change_id,
        older,
        newer,
    )
    result = subprocess.run(
        ["git", "log", "--format=%H", older + ".." + newer],
        capture_output=True,
        text=True,
        check=True,
    )
    commit_hashes = result.stdout.strip().split("\n")

    try:
        for commit_hash in tqdm(
            commit_hashes, desc="Checking change id exists", unit="item", leave=False
        ):
            change_ids_with_hashes = get_change_ids(commit_hash, commit_hash + "~1")
            for change, sha in change_ids_with_hashes:
                if change_id == change:
                    logging.debug("ChangeId: %s exists in %s", change_id, sha)
                    return True
    except ValueError as ve:
        logging.debug("Failed to inspect commit: %s", ve)

    return False


def is_branch_active(branch_name):
    """
    Checks if a branch is currently active.

    Args:
        branch_name (str): The name of the branch to check.

    Returns:
        bool: True if the branch is active, False otherwise.
    """
    try:
        result = subprocess.run(['git', 'branch', '--show-current'], capture_output=True, text=True, check=True)
        active_branch = result.stdout.strip()
        return branch_name == active_branch
    except subprocess.CalledProcessError as e:
        logging.error("An error occurred while checking the active branch: %s", e)
        return False



def delete_local_branch(branch_name):
    """Delete a local branch in the Git repository.

    Args:
        branch_name (str): The name of the local branch to be deleted.
    """
    logging.debug("Deleting local branch %s.", branch_name)

    if is_branch_active(branch_name):
        logging.error("Cannot branch: %s, it is currently an active branch!")

    try:
        subprocess.run(["git", "branch", "-D", branch_name], check=True)
        logging.info(
            "The local branch '%s' has been successfully deleted.", branch_name
        )
    except subprocess.CalledProcessError as e:
        logging.error("An error occurred: %s", e)


def is_subset_of(branch_name, main_branch):
    """Check if a branch is a subset of another branch.

    This function determines whether the specified branch is a subset of the main branch,
    by comparing the Change-Id values between the two branches.

    Args:
        branch_name (str): The name of the branch to be checked.
        main_branch (str): The name of the main branch to compare against.

    Returns:
        bool: True if the branch is a subset of the main branch, False otherwise.

    Raises:
        MissingChangeError: If a change is missing in the specified branch.
    """
    try:
        common_commit = find_last_common_commit(main_branch, branch_name)
        change_ids_branch_with_hashes = get_change_ids(branch_name, common_commit)
        for change_id, sha in change_ids_branch_with_hashes:
            if not check_change_id_exists(change_id, main_branch, common_commit):
                raise MissingChangeError(change_id, sha)
        return True
    except subprocess.CalledProcessError as e:
        logging.error(
            "Cannot determine state of branch %s due to error: %s", branch_name, e
        )
        return False


def find_deletable_branches(main_branch, dry_run):
    """Find branches that are deletable in the Git repository.

    This function searches for branches that can be deleted based on whether they are a subset of the main branch.

    Args:
        main_branch (str): The main branch to compare against.
        dry_run (bool): If True, performs a dry run without deleting branches.
    """
    logging.debug("Finding deletable branches.")
    result = subprocess.run(
        ["git", "branch"], capture_output=True, text=True, check=True
    )
    branches = result.stdout.strip().split("\n")

    for branch in branches:
        branch_name = branch.strip("* ")
        if branch_name:
            logging.info("Checking branch %s.", branch_name)
            try:
                if is_subset_of(branch_name, main_branch):
                    if not dry_run:
                        delete_local_branch(branch_name)
                    else:
                        logging.info("You can delete %s", branch_name)
            except MissingChangeError as me:
                logging.warning(
                    "Unable to delete branch %s since Change-Id: %s (sha: %s) is missing in %s",
                    branch_name,
                    me.change_id,
                    me.sha,
                    main_branch,
                )

            except subprocess.CalledProcessError as ce:
                logging.warning("Skipping %s, due to %s", branch_name, ce)


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="""Delete local git branches containing commits whose Change-Id exists in the main branch.

    This tool is useful when the `repo prune` command fails to detect merged branches. It will delete a branch
    if every commit contains a `Change-Id:` and the given `Change-Id` is in the main branch.
    """,
    )

    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose logging"
    )
    parser.add_argument(
        "--main",
        default="aosp/emu-master-dev",
        help="Set the main branch to use",
    )
    parser.add_argument(
        "-d",
        "--delete",
        action="store_true",
        default=False,
        help="Delete the branches, versus telling you that the can be deleted",
    )

    subparsers = parser.add_subparsers(help="commands", dest="command")

    # Command 'prune'
    prune_parser = subparsers.add_parser(
        "prune",
        help="Prune a single branch",
        description="Checks if the branch can be deleted, and if so will delete it.",
    )
    prune_parser.add_argument(
        "branch",
        help="The name of the branch to prune",
    )

    # Command 'prune-all'
    subparsers.add_parser(
        "prune-all",
        help="Prune all branches",
        description="Checks every branch if it can be deleted, and if so will delete it.",
    )

    args = parser.parse_args()

    if args.verbose:
        configure_logging(logging.DEBUG)
    else:
        configure_logging(logging.INFO)

    logging.debug("Verbose logging enabled.")
    main_branch = args.main
    dry_run = not args.delete

    logging.info("Using %s as the main branch", main_branch)
    if args.command == "prune":
        # Perform 'check' command
        try:
            if is_subset_of(args.branch, main_branch):
                print(f"{args.branch} can be deleted.")
                if args.delete:
                    delete_local_branch(args.branch)
        except MissingChangeError as me:
            print(
                f"Unable to delete branch {args.branch} since Change-Id: {me.change_id} (sha: {me.sha}) is missing in {main_branch}"
            )

    elif args.command == "prune-all":
        # Perform 'prune-all' command
        find_deletable_branches(main_branch, dry_run)

    else:
        # Handle the case when no command is specified
        parser.print_help()


if __name__ == "__main__":
    main()
