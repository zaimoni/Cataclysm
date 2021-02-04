(function () {
    if (window.location.hash) {
        const dest = 'a[name="' + window.location.hash.substr(1) + '"]';
        const test = new ZQuery(dest);
        const test2 = test.closest('tr');
        test2.css('backgroundColor','#CCCCCC');
    }
})();
