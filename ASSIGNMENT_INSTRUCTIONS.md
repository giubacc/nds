# Interview Assignment - Naive Distributed Storage

Hello, dear candidate! Thanks for taking the time to try this out.

The goal of this assignment is to assert (to some degree) your skills as a software engineer. You should focus on showcasing your skill at coding and problem solving. The solution is intentionally open-ended, to give you space for personal interpretation, but for all intents and purposes you can pretend that you're building a production-ready application!

You can develop the assignment in any common language you can demonstrate being comfortable coding with.

You're **allowed and encouraged** to use third party libraries, as long as they are free and open source. An effective developer knows what to build and what to reuse, but also how his/her tools work. Be prepared to answer some questions about those libraries, like why you chose them and what other alternatives you're familiar with.

As this is a code review process, please minimize generated code, as this might make our jobs as reviewers more difficult.

_Note: While we love open source at SUSE, please do not create a public repo with your assignment in! This test is only shared with people interviewing, and for obvious reasons we'd like it to remain this way._

## Instructions 

1. Clone this repository.
2. Create a pull request targeting the master branch of this repository.  
   This PR should contain setup instructions for your application and a breakdown of the technologies & packages you chose to use, why you chose to use them, and the design decisions you made.
3. Reply to the email thread you're having with our HR department telling them we can start reviewing your code.

## Requirements

Create a software system that can share data across at least two nodes across the network. A Naive Distributed Storage.
We'll refer to this as the "NDS" throughout the rest of this document.

- As a system administrator, I shall be able to run multiple NDS nodes and join them together, to form a cluster. 
- As a user, I shall be able to perform operations on any of the nodes composing the cluster.
- The user interface (e.g. CLI, remote requests, signals) to interact with the system is left as a free design decision.

### Constraints and details

- Only the set and read operations are to be implemented. No other operations are required.
- Existing data should be propagated on nodes joining after the data was set.
- Only one key is required to ever be read or written. The ability to address multiple pieces data of data, like in a key-value store, is not required.
- There is no need to implement multiple datatypes. You can just pick one, like integers or strings.
- The NDS _must not_ rely on an completely separate database system, i.e. running alongside a RDBMS like MySQL or a key-value store like Redis. You can, however, use third party libraries to implement well-known protocols and algorithms within the processes ran by the NDS.
- The network protocol used by the nodes is left as a free design decision.
- What happens in case of failure of one of the nodes, or the network connecting them, is left as a free design decision.

### Example

Here is an example of an hypotetical local CLI user interaction, which is just indicative of the desired result but it must not taken in any way as prescriptive:
```shell
sysadmin@node1 $: nds --start
user@node1 $: nds set 123

sysadmin@node2 $: nds --join node1
user@node2 $: nds read
123
```
Your implementation might very well completely differ from the above.

## What to expect from the review

The review of your solution will happen in two phases:
1. First, you'll have a chance to interact with the same team members you would be joining, in a normal, asynchronous code review, like you would do on GitHub, every day.
2. After the asynchronous review is done, we'll schedule a live chat with team, where further in-depth questions might be asked, and also to give you the chance to meet your potential future colleagues!

## Good luck!

Code something awesome!
