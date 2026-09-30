plugins {
    alias(libs.plugins.kotlin.jvm)
    application
}

kotlin {
    jvmToolchain(25)
}

dependencies {
    implementation(project(":core"))
    implementation(project(":oracle"))
}

application {
    mainClass = "ugh.desktop.MainKt"
    applicationDefaultJvmArgs = listOf("-Dsun.java2d.uiScale=1")
}

tasks.named<JavaExec>("run") {
    workingDir = rootProject.projectDir
}
