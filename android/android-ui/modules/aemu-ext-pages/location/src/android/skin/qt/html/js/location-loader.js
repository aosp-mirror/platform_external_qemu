$('body').append('<div style="" id="loadingDiv"><div class="loader">Loading...</div></div>');
$(window).on('load', function () {
    setTimeout(removeLoader, 2000); // wait for page load PLUS 2 seconds.
});
function removeLoader() {
    $("#loadingDiv").fadeOut(500, function () {
        // fadeOut complete. Remove the loading div
        $("#loadingDiv").remove();
    });
}
