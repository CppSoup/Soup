# Design Requirements/Goals
The below Requirements and Goals are set down as guidelines to better align engineering decisions and prioritization of work.

## Requirements
The following set of requirements cannot be compromised. The order does not indicate a priority; but, the final system would be deemed a failure if we are unable to fulfill any one of them.

* **Reproducible** - Core to any build system is the requirement that builds be **deterministic** and **reproducible**. No matter how well a system is designed and implemented, teams will not be able to utilize it unless they can trust that it will always produce the same result independent of who builds it, where they build it and when. Reproducible builds requires that one project cannot influence or be influenced by another build, intentionally or by accident, except through explicit structured channels.

* **Extensible** - A build system should be able to support the requirements of **all** projects. It should strive to work "out of the box" for a majority of scenarios, but must have an extensibility framework that allows build architects to write their own custom build logic when the built in functionality does not meet their needs.

* **Secure** - By its nature, shared code and extensible frameworks open itself up to security concerns when executing arbitrary code written by external teams. While this is the same concern present when consuming any open source project, and the community should take care to use only trusted sources for the projects their use, the build system will limit the functionality available to the build runtime to prevent access to the developers machine when not required. The build system must also enable safe development practices to ensure code can be built and released within a compliant environment to guarantee what is published matches the input code.

## Goals
While the goals are not hard requirements, they are always kept at the forefront when making any design or implementation decision. These items **are** in priority order:

1. **Collaborative** - Writing code is very rarely done in isolation. The largest goal for this build system is to be able to work seamlessly within a team and with external dependencies.

1. **Fast** - The inner developer loop is very important to the productivity of an engineer. To this end, the build system should focus heavily on the performance of an incremental build and ensure the full build is as fast as possible.

1. **Simple** - When fulfilling the above requirements the secondary priority is always simplicity and usability. This means that the standard user will get the best experience possible for both setup and usage. Some extra complexity is allowed in exchange for performance gains in the internal implementation and the extensibility framework.

1. **Customizable** - How a project is built is often a matter of personal preference (or legacy requirement). Where allowable, the build system should be customizable to allow for overriding default settings so it does not conflict with the ability to easily build single projects as a part of the greater ecosystem.