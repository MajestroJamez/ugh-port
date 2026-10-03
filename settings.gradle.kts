rootProject.name = "ugh"

pluginManagement {
    repositories {
        gradlePluginPortal()
        mavenCentral()
    }
}

dependencyResolutionManagement {
    repositories {
        mavenCentral()
    }
}

// the projects live in the folders of the project steps (README.md)
include("extractor", "core", "oracle", "verify", "desktop")
project(":core").projectDir = file("3_kotlin_port/core")
project(":oracle").projectDir = file("3_kotlin_port/oracle")
project(":desktop").projectDir = file("3_kotlin_port/desktop")
project(":extractor").projectDir = file("4_test_data/extractor")
project(":verify").projectDir = file("4_test_data/verify")
