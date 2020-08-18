const csfd = require('csfd-api')

const PROCESSING_CSFD = 'csfd'
const PROCESSING_IMDB = 'imdb'

// Fire it up
return main()

/**
 * Main function.
 *
 * Code is designed to search and return movie detail in one shot, it is better for
 * performance.
 */
async function main() {
    // Validations
    if (process.argv.length !== 4)
        return badParameters()

    // Prepare data
    const movieDatabase = process.argv[2]
    const searchQueryString = process.argv[3]

    switch (movieDatabase) {
        case PROCESSING_CSFD:
            // TODO non strigified version when env. var. eg pretty passed or by cmd line parameter silverqx
            console.log(JSON.stringify(await getCsfdMovieDetail(searchQueryString)))
            break
        case PROCESSING_IMDB:
            console.log(JSON.stringify(await getImdbMovieDetail(searchQueryString)))
            break
        default:
            return badParameters()
    }

    return 0
}

async function getCsfdMovieDetail(searchQueryString) {
    const searchResult = await csfd.search(searchQueryString)
    return await csfd.film(searchResult.films[0].id)
}

async function getImdbMovieDetail(searchQueryString) {
    return {}
}

/**
 * Bad parameters passed on the command line.
 */
function badParameters() {
    console.error(new Error('Bad parameters format'))
    process.exitCode = 1
    return 1
}
